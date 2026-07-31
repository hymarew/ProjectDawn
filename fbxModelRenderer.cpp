#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "main.h"
#include "renderer.h"
#include "fbxModelRenderer.h"
#include "DirectXTex.h"

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#pragma comment(lib, "assimp-vc143-mt.lib")

#include <vector>
#include <cmath>

using namespace DirectX;


std::unordered_map<std::string, MODEL*>       FbxModelRenderer::m_ModelPool;
std::unordered_map<std::string, FbxSkinData*> FbxModelRenderer::m_SkinPool;


// Assimpの列ベクトル規約(平行移動が最後の列)からDirectXの行ベクトル規約へ転置して変換する(skeleton.cppと同じ規約)。
static XMMATRIX ToXMMatrix(const aiMatrix4x4& m)
{
	return XMMatrixSet(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4);
}


//======================================================================
// Phase4: アニメーション補間 + 階層をたどったワールド変換 + 最終スキニング行列の計算
//======================================================================

static XMVECTOR InterpolateVec3(const std::vector<Vec3Key>& keys, float time, XMVECTOR defaultValue)
{
	if (keys.empty()) return defaultValue;
	if (keys.size() == 1 || time <= keys.front().Time) return XMLoadFloat3(&keys.front().Value);
	if (time >= keys.back().Time) return XMLoadFloat3(&keys.back().Value);

	for (size_t i = 0; i + 1 < keys.size(); i++)
	{
		if (time >= keys[i].Time && time <= keys[i + 1].Time)
		{
			float span = keys[i + 1].Time - keys[i].Time;
			float t = (span > 0.0f) ? (time - keys[i].Time) / span : 0.0f;
			return XMVectorLerp(XMLoadFloat3(&keys[i].Value), XMLoadFloat3(&keys[i + 1].Value), t);
		}
	}
	return XMLoadFloat3(&keys.back().Value);
}


static XMVECTOR InterpolateQuat(const std::vector<QuatKey>& keys, float time)
{
	if (keys.empty()) return XMQuaternionIdentity();
	if (keys.size() == 1 || time <= keys.front().Time) return XMLoadFloat4(&keys.front().Value);
	if (time >= keys.back().Time) return XMLoadFloat4(&keys.back().Value);

	for (size_t i = 0; i + 1 < keys.size(); i++)
	{
		if (time >= keys[i].Time && time <= keys[i + 1].Time)
		{
			float span = keys[i + 1].Time - keys[i].Time;
			float t = (span > 0.0f) ? (time - keys[i].Time) / span : 0.0f;

			XMVECTOR a = XMLoadFloat4(&keys[i].Value);
			XMVECTOR b = XMLoadFloat4(&keys[i + 1].Value);

			// クォータニオンはq/-qが同じ回転を表す(二重被覆)。符号が逆転していると
			// XMQuaternionSlerpは最短経路ではなく遠回りの補間をしてしまい、
			// 大きく動く脚などの関節で不自然に折れ曲がって見える。内積が負なら片方を反転して揃える。
			if (XMVectorGetX(XMQuaternionDot(a, b)) < 0.0f)
				b = XMVectorNegate(b);

			return XMQuaternionSlerp(a, b, t);
		}
	}
	return XMLoadFloat4(&keys.back().Value);
}


// 指定ボーンのアニメーションチャンネルから、指定時刻におけるローカル変換(Scale * Rotation * Translation)を作る。
// 既存コード(Player::Draw等)と同じ「scale * rot * trans」の順序(行ベクトル規約)。
//
// 位置・回転とも「バインドポーズ + (アニメーション先頭フレームからの変化量)」という差分方式で適用する。
// 理由: FBXのピボット分解で、HipsのようなボーンではTranslationが、LeftUpLegのようなボーンではPreRotationが
// それぞれ親側の補助ノード("$AssimpFbx$_Translation"/"$AssimpFbx$_PreRotation")へ逃がされており、
// ボーン自身のバインド値は実質(0,0,0)/単位回転になっている。アニメーションの絶対値をそのまま使うと、
// 親側の補助ノードが持つ静的な値と二重に加算/合成されてしまい、位置では手足が分解し、
// 回転では脚が変な方向へ大きく折れ曲がる(PreRotationとほぼ同じ大きさの回転が二重にかかるため)。
// 差分方式なら「アニメーション先頭フレームからの変化量」だけを取り出すため、この分解の違いに影響されない。
static XMMATRIX ComposeLocalTransform(const BoneAnimation& anim, float time, const XMMATRIX& bindLocal)
{
	XMVECTOR bindTranslation = XMVectorSetW(bindLocal.r[3], 0.0f);
	XMVECTOR bindRotation    = XMQuaternionRotationMatrix(bindLocal);

	// 位置: バインド位置 + (現在値 - 先頭キー値)
	XMVECTOR translation = bindTranslation;
	if (!anim.PositionKeys.empty())
	{
		XMVECTOR firstPos = XMLoadFloat3(&anim.PositionKeys[0].Value);
		XMVECTOR curPos   = InterpolateVec3(anim.PositionKeys, time, firstPos);
		translation = XMVectorAdd(bindTranslation, XMVectorSubtract(curPos, firstPos));
	}

	// 回転: バインド回転 * (先頭キー回転の逆 * 現在の回転) ※行ベクトル規約でA*Bは「Aを先に適用」
	XMMATRIX bindRotMatrix = XMMatrixRotationQuaternion(bindRotation);
	XMMATRIX rotMatrix = bindRotMatrix;
	if (!anim.RotationKeys.empty())
	{
		XMVECTOR firstRot = XMLoadFloat4(&anim.RotationKeys[0].Value);
		XMVECTOR curRot   = InterpolateQuat(anim.RotationKeys, time);
		XMMATRIX firstRotMatrixInv = XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(firstRot));
		XMMATRIX curRotMatrix      = XMMatrixRotationQuaternion(curRot);
		rotMatrix = bindRotMatrix * firstRotMatrixInv * curRotMatrix;
	}

	XMVECTOR scale = InterpolateVec3(anim.ScaleKeys, time, XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	XMMATRIX s = XMMatrixScalingFromVector(scale);
	XMMATRIX t = XMMatrixTranslationFromVector(translation);

	return s * rotMatrix * t;
}


// Skeleton(バインドポーズ + 階層)とAnimationClip(省略時はバインドポーズのまま)から、
// GPUへ送る最終スキニング行列配列(HLSLのBoneMatrices[MAX_BONES]用、Transpose済み)を計算する。
static void ComputeSkinnedBoneMatrices(const Skeleton& skeleton, const AnimationClip* clip, float time, std::vector<XMMATRIX>& outGpuMatrices)
{
	std::vector<XMMATRIX> localTransforms(skeleton.Bones.size());

	for (size_t i = 0; i < skeleton.Bones.size(); i++)
	{
		const Bone& bone = skeleton.Bones[i];
		const BoneAnimation* boneAnim = clip ? clip->FindBoneAnimation(bone.Name) : nullptr;

		localTransforms[i] = boneAnim ? ComposeLocalTransform(*boneAnim, time, bone.LocalBindMatrix) : bone.LocalBindMatrix;
	}

	// 親は必ず子より前のインデックスに格納されている(Skeleton::BuildFromSceneの構築順)
	std::vector<XMMATRIX> globalTransforms(skeleton.Bones.size());
	for (size_t i = 0; i < skeleton.Bones.size(); i++)
	{
		const Bone& bone = skeleton.Bones[i];
		globalTransforms[i] = (bone.ParentIndex < 0)
			? localTransforms[i]
			: localTransforms[i] * globalTransforms[bone.ParentIndex];
	}

	outGpuMatrices.assign(FbxSkinData::MAX_BONES, XMMatrixIdentity());
	for (size_t i = 0; i < skeleton.Bones.size(); i++)
	{
		const Bone& bone = skeleton.Bones[i];
		if (!bone.HasOffset) continue;
		if (bone.GpuIndex < 0 || bone.GpuIndex >= FbxSkinData::MAX_BONES) continue;

		// HLSL側のcbufferはデフォルトでcolumn-major解釈のため、
		// 他の行列(World/View/Projection等)と同様にアップロード前にTransposeする
		outGpuMatrices[bone.GpuIndex] = XMMatrixTranspose(bone.OffsetMatrix * globalTransforms[i]);
	}
}


void FbxModelRenderer::Draw()
{
	UINT stride = m_SkinData ? sizeof(VERTEX_3D_SKIN) : sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_Model->VertexBuffer, &stride, &offset);
	Renderer::GetDeviceContext()->IASetIndexBuffer(m_Model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_SkinData)
		Renderer::GetDeviceContext()->VSSetConstantBuffers(8, 1, &m_BoneMatrixBuffer);

	for (unsigned int i = 0; i < m_Model->SubsetNum; i++)
	{
		Renderer::SetMaterial(m_Model->SubsetArray[i].Material.Material);

		if (m_Model->SubsetArray[i].Material.Texture)
			Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Model->SubsetArray[i].Material.Texture);

		Renderer::GetDeviceContext()->DrawIndexed(m_Model->SubsetArray[i].IndexNum, m_Model->SubsetArray[i].StartIndex, 0);
	}
}


void FbxModelRenderer::DrawShadow()
{
	// シャドウパスは既存のShadowDepthVS(POSITION/NORMALのみ参照)を使い回すため、
	// スキニング有無に関わらずVERTEX_3D_SKINの先頭レイアウトがVERTEX_3Dと一致していれば問題ない。
	// (Phase2はバインドポーズ固定のため、影も静止ポーズのままで正しい)
	UINT stride = m_SkinData ? sizeof(VERTEX_3D_SKIN) : sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_Model->VertexBuffer, &stride, &offset);
	Renderer::GetDeviceContext()->IASetIndexBuffer(m_Model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (unsigned int i = 0; i < m_Model->SubsetNum; i++)
	{
		Renderer::GetDeviceContext()->DrawIndexed(m_Model->SubsetArray[i].IndexNum, m_Model->SubsetArray[i].StartIndex, 0);
	}
}


void FbxModelRenderer::UnloadAll()
{
	for (std::pair<const std::string, MODEL*> pair : m_ModelPool)
	{
		pair.second->VertexBuffer->Release();
		pair.second->IndexBuffer->Release();

		for (unsigned int i = 0; i < pair.second->SubsetNum; i++)
		{
			if (pair.second->SubsetArray[i].Material.Texture)
				pair.second->SubsetArray[i].Material.Texture->Release();
		}

		delete[] pair.second->SubsetArray;
		delete pair.second;
	}
	m_ModelPool.clear();

	for (std::pair<const std::string, FbxSkinData*> pair : m_SkinPool)
	{
		delete pair.second;
	}
	m_SkinPool.clear();
}


void FbxModelRenderer::Uninit()
{
	// m_BoneMatrixBufferはインスタンス固有(m_ModelPool/m_SkinPoolのように使い回さない)ため、ここで解放する
	if (m_BoneMatrixBuffer)
	{
		m_BoneMatrixBuffer->Release();
		m_BoneMatrixBuffer = nullptr;
	}
}


void FbxModelRenderer::Update(float dt)
{
	if (!m_SkinData || !m_HasAnimation) return;

	m_AnimTime += dt;
	if (m_CurrentClip.Duration > 0.0f)
		m_AnimTime = fmodf(m_AnimTime, m_CurrentClip.Duration);

	std::vector<XMMATRIX> gpuMatrices;
	ComputeSkinnedBoneMatrices(m_SkinData->Skeleton, &m_CurrentClip, m_AnimTime, gpuMatrices);

	Renderer::GetDeviceContext()->UpdateSubresource(m_BoneMatrixBuffer, 0, nullptr, gpuMatrices.data(), 0, 0);
}


void FbxModelRenderer::PlayAnimation(const char *FileName)
{
	if (!m_SkinData) return; // スキニングモデルでなければ何もしない

	std::vector<AnimationClip> clips = LoadAnimationClips(FileName);
	if (clips.empty()) return;

	m_CurrentClip = clips[0];
	m_AnimTime = 0.0f;
	m_HasAnimation = true;
}


void FbxModelRenderer::Load(const char *FileName)
{
	bool alreadyCached = m_ModelPool.count(FileName) > 0;

	if (alreadyCached)
	{
		m_Model = m_ModelPool[FileName];

		auto it = m_SkinPool.find(FileName);
		m_SkinData = (it != m_SkinPool.end()) ? it->second : nullptr;
	}
	else
	{
		m_Model = new MODEL;
		FbxSkinData* skinData = nullptr;
		LoadModel(FileName, m_Model, &skinData);

		m_ModelPool[FileName] = m_Model;
		m_SkinData = skinData;

		if (skinData)
			m_SkinPool[FileName] = skinData;
	}

	// ボーン行列バッファはインスタンス固有(同じモデルを複数のGameObjectが使っても
	// 再生中のアニメーション・時刻はそれぞれ別に持つ必要があるため、プールせず毎回生成する)
	if (m_SkinData)
	{
		std::vector<XMMATRIX> gpuMatrices;
		ComputeSkinnedBoneMatrices(m_SkinData->Skeleton, nullptr, 0.0f, gpuMatrices); // 初期状態はバインドポーズ

		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(XMMATRIX) * FbxSkinData::MAX_BONES;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = gpuMatrices.data();

		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_BoneMatrixBuffer);
	}
}


void FbxModelRenderer::LoadModel(const char *FileName, MODEL *Model, FbxSkinData** OutSkinData)
{
	// aiProcess_Triangulate      : 全ポリゴンを三角形化(既存パイプラインは三角形リスト前提)
	// aiProcess_ConvertToLeftHanded : DirectX(左手座標系)へ変換
	// aiProcess_GenNormals       : 法線が無いメッシュに備える(保険)
	// aiProcess_LimitBoneWeights : 頂点あたりのボーン影響数を4以下に制限・正規化する(VERTEX_3D_SKINの枠数と一致させる)
	const aiScene* scene = aiImportFile(FileName,
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_GenNormals | aiProcess_LimitBoneWeights);

	if (!scene)
	{
		MessageBoxA(nullptr, aiGetErrorString(), "FBX Load Error", MB_OK);
		assert(scene);
		return;
	}

	char dir[MAX_PATH];
	strcpy(dir, FileName);
	PathRemoveFileSpec(dir);

	bool hasBones = false;
	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
		if (scene->mMeshes[m]->mNumBones > 0)
			hasBones = true;

	// ==================================================
	// スキニング情報(Skeleton)の構築。
	// メッシュ本体より先に、各ボーンのGPU行列インデックスを確定させておく。
	// ==================================================
	Skeleton skeleton;
	if (hasBones)
	{
		skeleton.BuildFromScene(scene);

		int gpuIndex = 0;
		for (unsigned int m = 0; m < scene->mNumMeshes; m++)
		{
			aiMesh* mesh = scene->mMeshes[m];
			for (unsigned int b = 0; b < mesh->mNumBones; b++)
			{
				aiBone* bone = mesh->mBones[b];
				int boneIndex = skeleton.FindBoneIndex(bone->mName.C_Str());
				if (boneIndex < 0) continue; // 通常は起こらないはず(ノード階層に必ず存在する)

				Bone& skelBone = skeleton.Bones[boneIndex];
				if (!skelBone.HasOffset)
				{
					skelBone.HasOffset   = true;
					skelBone.OffsetMatrix = ToXMMatrix(bone->mOffsetMatrix);
					skelBone.GpuIndex     = gpuIndex++;
				}
			}
		}
		skeleton.GpuBoneCount = gpuIndex;
	}

	// 全メッシュを単一の頂点/インデックスバッファへ結合し、メッシュ単位でSUBSET(描画範囲+マテリアル)に分ける。
	unsigned int totalVertexNum = 0;
	unsigned int totalIndexNum = 0;
	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		totalVertexNum += scene->mMeshes[m]->mNumVertices;
		totalIndexNum += scene->mMeshes[m]->mNumFaces * 3;
	}

	// スキニング無し(Phase1)と有り(Phase2)で頂点フォーマットが異なるため配列を出し分ける。
	VERTEX_3D*      vertexArray     = hasBones ? nullptr : new VERTEX_3D[totalVertexNum];
	VERTEX_3D_SKIN* vertexArraySkin = hasBones ? new VERTEX_3D_SKIN[totalVertexNum] : nullptr;
	unsigned int* indexArray = new unsigned int[totalIndexNum];

	Model->SubsetArray = new SUBSET[scene->mNumMeshes];
	Model->SubsetNum = scene->mNumMeshes;

	unsigned int vertexOffset = 0;
	unsigned int indexOffset = 0;

	for (unsigned int m = 0; m < scene->mNumMeshes; m++)
	{
		aiMesh* mesh = scene->mMeshes[m];

		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			XMFLOAT3 position = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);

			XMFLOAT3 normal = mesh->HasNormals()
				? XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
				: XMFLOAT3(0.0f, 1.0f, 0.0f);

			XMFLOAT2 texCoord = mesh->HasTextureCoords(0)
				? XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
				: XMFLOAT2(0.0f, 0.0f);

			if (hasBones)
			{
				VERTEX_3D_SKIN& vertex = vertexArraySkin[vertexOffset + v];
				vertex.Position = position;
				vertex.Normal   = normal;
				vertex.TexCoord = texCoord;
				vertex.Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				vertex.BoneIndices[0] = vertex.BoneIndices[1] = vertex.BoneIndices[2] = vertex.BoneIndices[3] = 0;
				vertex.BoneWeights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				VERTEX_3D& vertex = vertexArray[vertexOffset + v];
				vertex.Position = position;
				vertex.Normal   = normal;
				vertex.TexCoord = texCoord;
				vertex.Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// ボーンごとの頂点ウェイトをVERTEX_3D_SKINへ書き込む(aiProcess_LimitBoneWeightsにより最大4件/頂点)
		if (hasBones)
		{
			std::vector<int> nextSlot(mesh->mNumVertices, 0);

			for (unsigned int b = 0; b < mesh->mNumBones; b++)
			{
				aiBone* bone = mesh->mBones[b];
				int boneIndex = skeleton.FindBoneIndex(bone->mName.C_Str());
				if (boneIndex < 0) continue;
				int gpuIndex = skeleton.Bones[boneIndex].GpuIndex;

				for (unsigned int w = 0; w < bone->mNumWeights; w++)
				{
					unsigned int vid = bone->mWeights[w].mVertexId;
					float weight = bone->mWeights[w].mWeight;
					if (weight <= 0.0f) continue;

					int slot = nextSlot[vid];
					if (slot >= 4) continue; // 4件を超える分は切り捨て(通常はLimitBoneWeightsで発生しない)

					VERTEX_3D_SKIN& vertex = vertexArraySkin[vertexOffset + vid];
					vertex.BoneIndices[slot] = (UINT)gpuIndex;
					reinterpret_cast<float*>(&vertex.BoneWeights)[slot] = weight;
					nextSlot[vid] = slot + 1;
				}
			}

			// どのボーンにも属さない頂点(スキンウェイトを持たない孤立頂点)は、
			// そのまま静止させるためウェイトをこのメッシュ自身のルート変換(単位行列)へ逃がす。
			// (Y Botのような全身スキン済みメッシュでは基本的に発生しない安全策)
			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				VERTEX_3D_SKIN& vertex = vertexArraySkin[vertexOffset + v];
				float sum = vertex.BoneWeights.x + vertex.BoneWeights.y + vertex.BoneWeights.z + vertex.BoneWeights.w;
				if (sum <= 0.0f)
				{
					vertex.BoneIndices[0] = 0;
					vertex.BoneWeights = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
				}
			}
		}

		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			const aiFace& face = mesh->mFaces[f];

			indexArray[indexOffset + f * 3 + 0] = vertexOffset + face.mIndices[0];
			indexArray[indexOffset + f * 3 + 1] = vertexOffset + face.mIndices[1];
			indexArray[indexOffset + f * 3 + 2] = vertexOffset + face.mIndices[2];
		}

		SUBSET& subset = Model->SubsetArray[m];
		subset.StartIndex = indexOffset;
		subset.IndexNum = mesh->mNumFaces * 3;
		subset.Material.Texture = nullptr;

		//==============================
		// マテリアル読み込み
		//==============================
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
		aiColor4D ambient(0.2f, 0.2f, 0.2f, 1.0f);
		aiColor4D specular(0.0f, 0.0f, 0.0f, 1.0f);
		float shininess = 0.0f;

		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
		material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
		material->Get(AI_MATKEY_SHININESS, shininess);

		subset.Material.Material.Diffuse = XMFLOAT4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
		subset.Material.Material.Ambient = XMFLOAT4(ambient.r, ambient.g, ambient.b, ambient.a);
		subset.Material.Material.Specular = XMFLOAT4(specular.r, specular.g, specular.b, specular.a);
		subset.Material.Material.Emission = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		subset.Material.Material.Shininess = shininess;

		//==============================
		// テクスチャ読み込み(外部ファイル参照 / FBX埋め込みの両方に対応)
		//==============================
		aiString texPath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS && texPath.length > 0)
		{
			TexMetadata metadata;
			ScratchImage image;
			bool loaded = false;

			if (texPath.data[0] == '*')
			{
				// FBX内部に埋め込まれたテクスチャ("*0"のようにインデックスで参照される)
				int texIndex = atoi(texPath.data + 1);
				if (texIndex >= 0 && texIndex < (int)scene->mNumTextures)
				{
					aiTexture* embeddedTex = scene->mTextures[texIndex];
					if (embeddedTex->mHeight == 0)
					{
						// 圧縮画像(png/jpg等)がそのままメモリ上に格納されている
						if (SUCCEEDED(LoadFromWICMemory((const uint8_t*)embeddedTex->pcData, embeddedTex->mWidth, WIC_FLAGS_NONE, &metadata, image)))
							loaded = true;
					}
				}
			}
			else
			{
				// 外部ファイル参照。FBXと同じフォルダにあるものとして解決する。
				char path[MAX_PATH];
				strcpy(path, dir);
				strcat(path, "\\");
				strcat(path, texPath.data);

				wchar_t wpath[MAX_PATH];
				mbstowcs(wpath, path, MAX_PATH);

				if (SUCCEEDED(LoadFromWICFile(wpath, WIC_FLAGS_NONE, &metadata, image)))
					loaded = true;
			}

			if (loaded)
			{
				ScratchImage mipChain;
				if (SUCCEEDED(GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain)))
					CreateShaderResourceView(Renderer::GetDevice(), mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), &subset.Material.Texture);
				else
					CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &subset.Material.Texture);
			}
		}

		subset.Material.Material.TextureEnable = (subset.Material.Texture != nullptr);

		vertexOffset += mesh->mNumVertices;
		indexOffset += mesh->mNumFaces * 3;
	}

	//==============================
	// 頂点バッファ生成
	//==============================
	{
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = hasBones ? (sizeof(VERTEX_3D_SKIN) * totalVertexNum) : (sizeof(VERTEX_3D) * totalVertexNum);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = hasBones ? (void*)vertexArraySkin : (void*)vertexArray;

		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &Model->VertexBuffer);
	}

	//==============================
	// インデックスバッファ生成
	//==============================
	{
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(unsigned int) * totalIndexNum;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = indexArray;

		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &Model->IndexBuffer);
	}

	delete[] vertexArray;
	delete[] vertexArraySkin;
	delete[] indexArray;

	// Skeleton(骨格の共有データ)はここで確定させる。
	// GPUへ送るボーン行列バッファはインスタンスごとに再生状態が異なるため、Load()側で生成する。
	if (hasBones)
	{
		FbxSkinData* skinData = new FbxSkinData();
		skinData->Skeleton = skeleton;
		*OutSkinData = skinData;
	}

	aiReleaseImport(scene);
}
