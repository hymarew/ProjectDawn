#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "main.h"
#include "renderer.h"
#include "fbxModelRenderer.h"
#include "DirectXTex.h"

#include <fbxsdk.h>
#pragma comment(lib, "libfbxsdk-mt.lib")
#pragma comment(lib, "libxml2-mt.lib")
#pragma comment(lib, "zlib-mt.lib")
#pragma comment(lib, "Bcrypt.lib") // libxml2の乱数生成(xmlInitRandom)がBCryptGenRandomを要求する

#include <algorithm>
#include <vector>

using namespace DirectX;


std::unordered_map<std::string, MODEL*>       FbxModelRenderer::m_ModelPool;
std::unordered_map<std::string, FbxSkinData*> FbxModelRenderer::m_SkinPool;


// FbxAMatrixをT/R/Sへ分解してからDirectXの行ベクトル規約(Scale*Rotation*Translation)で組み直す。
// skeleton.cppのToXMMatrixと同じ実装(GetColumn直読み+転置方式は平行移動が失われる不具合があったため)。
static XMMATRIX ToXMMatrix(const FbxAMatrix& m)
{
	FbxVector4    t = m.GetT();
	FbxQuaternion q = m.GetQ();
	FbxVector4    s = m.GetS();

	XMMATRIX sm = XMMatrixScaling((float)s[0], (float)s[1], (float)s[2]);
	XMMATRIX rm = XMMatrixRotationQuaternion(XMVectorSet((float)q[0], (float)q[1], (float)q[2], (float)q[3]));
	XMMATRIX tm = XMMatrixTranslation((float)t[0], (float)t[1], (float)t[2]);

	return sm * rm * tm;
}


//======================================================================
// Phase4: アニメーション補間(キーフレーム間の線形補間/球面線形補間)
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
			// XMQuaternionSlerpは最短経路ではなく遠回りの補間をしてしまい、関節が不自然に折れ曲がって見える。
			if (XMVectorGetX(XMQuaternionDot(a, b)) < 0.0f)
				b = XMVectorNegate(b);

			return XMQuaternionSlerp(a, b, t);
		}
	}
	return XMLoadFloat4(&keys.back().Value);
}


// 指定ボーンのアニメーションチャンネルから、指定時刻におけるローカル変換(Scale * Rotation * Translation)を作る。
// FbxAnimEvaluator::GetNodeLocalTransformで焼き込んだキー(animationClip.cpp)は、FBXのピボット分解
// (PreRotation等)をFBX SDK側で解決済みの完全なローカル変換のため、Assimp版のような
// 「バインドポーズ + 先頭フレームからの差分」の補正は不要で、キーの値をそのまま使える。
static XMMATRIX ComposeLocalTransform(const BoneAnimation& anim, float time)
{
	XMVECTOR position = InterpolateVec3(anim.PositionKeys, time, XMVectorZero());
	XMVECTOR rotation = InterpolateQuat(anim.RotationKeys, time);
	XMVECTOR scale    = InterpolateVec3(anim.ScaleKeys, time, XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	XMMATRIX s = XMMatrixScalingFromVector(scale);
	XMMATRIX r = XMMatrixRotationQuaternion(rotation);
	XMMATRIX t = XMMatrixTranslationFromVector(position);

	return s * r * t;
}


//======================================================================
// Skeleton(バインドポーズ + 階層)から、階層をたどって各ボーンのモデル空間ワールド変換を計算する。
// clipがnullptrならバインドポーズのまま(Phase2相当)、指定されていれば指定時刻の再生姿勢(Phase4)。
//======================================================================
static void ComputeGlobalTransforms(const Skeleton& skeleton, const AnimationClip* clip, float time, std::vector<XMMATRIX>& outGlobalTransforms)
{
	outGlobalTransforms.resize(skeleton.Bones.size());

	// 親は必ず子より前のインデックスに格納されている(Skeleton::BuildFromSceneの構築順)
	for (size_t i = 0; i < skeleton.Bones.size(); i++)
	{
		const Bone& bone = skeleton.Bones[i];
		const BoneAnimation* boneAnim = clip ? clip->FindBoneAnimation(bone.Name) : nullptr;

		XMMATRIX localTransform = boneAnim ? ComposeLocalTransform(*boneAnim, time) : bone.LocalBindMatrix;

		outGlobalTransforms[i] = (bone.ParentIndex < 0)
			? localTransform
			: localTransform * outGlobalTransforms[bone.ParentIndex];
	}
}


static void ComputeGpuBoneMatrices(const Skeleton& skeleton, const std::vector<XMMATRIX>& globalTransforms, std::vector<XMMATRIX>& outGpuMatrices)
{
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
		delete pair.second;
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


bool FbxModelRenderer::GetCurrentBonePositions(std::vector<XMFLOAT3>& outPositions, std::vector<int>& outParentIndices, std::vector<bool>& outHasOffset) const
{
	if (!m_SkinData || m_LastGlobalTransforms.empty()) return false;

	const Skeleton& skeleton = m_SkinData->Skeleton;

	outPositions.resize(skeleton.Bones.size());
	outParentIndices.resize(skeleton.Bones.size());
	outHasOffset.resize(skeleton.Bones.size());

	for (size_t i = 0; i < skeleton.Bones.size(); i++)
	{
		XMStoreFloat3(&outPositions[i], m_LastGlobalTransforms[i].r[3]);
		outParentIndices[i] = skeleton.Bones[i].ParentIndex;
		outHasOffset[i] = skeleton.Bones[i].HasOffset;
	}

	return true;
}


void FbxModelRenderer::Update(float dt)
{
	if (!m_SkinData || !m_HasAnimation) return;

	m_AnimTime += dt;
	if (m_CurrentClip.Duration > 0.0f)
		m_AnimTime = fmodf(m_AnimTime, m_CurrentClip.Duration); // ループ再生

	ComputeGlobalTransforms(m_SkinData->Skeleton, &m_CurrentClip, m_AnimTime, m_LastGlobalTransforms);

	std::vector<XMMATRIX> gpuMatrices;
	ComputeGpuBoneMatrices(m_SkinData->Skeleton, m_LastGlobalTransforms, gpuMatrices);

	Renderer::GetDeviceContext()->UpdateSubresource(m_BoneMatrixBuffer, 0, nullptr, gpuMatrices.data(), 0, 0);
}


int FbxModelRenderer::GetMatchedAnimationBoneCount() const
{
	if (!m_SkinData || !m_HasAnimation) return 0;

	int matched = 0;
	for (const Bone& bone : m_SkinData->Skeleton.Bones)
		if (m_CurrentClip.FindBoneAnimation(bone.Name) != nullptr)
			matched++;

	return matched;
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

	// ボーン行列バッファはインスタンス固有(同じモデルを複数のGameObjectが使う場合、
	// Phase3/4では再生中のアニメーション時刻がそれぞれ別になる必要があるため、プールせず毎回生成する)
	if (m_SkinData)
	{
		ComputeGlobalTransforms(m_SkinData->Skeleton, nullptr, 0.0f, m_LastGlobalTransforms); // 初期状態はバインドポーズ

		std::vector<XMMATRIX> gpuMatrices;
		ComputeGpuBoneMatrices(m_SkinData->Skeleton, m_LastGlobalTransforms, gpuMatrices);

		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(XMMATRIX) * FbxSkinData::MAX_BONES;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = gpuMatrices.data();

		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_BoneMatrixBuffer);
	}
}


static void CollectMeshNodesRecursive(FbxNode* node, std::vector<FbxNode*>& outMeshNodes)
{
	FbxNodeAttribute* attr = node->GetNodeAttribute();
	if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh)
		outMeshNodes.push_back(node);

	for (int i = 0; i < node->GetChildCount(); i++)
		CollectMeshNodesRecursive(node->GetChild(i), outMeshNodes);
}


// 頂点あたり最大4影響のBLENDINDICES/BLENDWEIGHT枠に合わせて上位4件のみ残し、合計が1になるよう正規化する。
// (AssimpのaiProcess_LimitBoneWeightsに相当する処理をFBX SDK側で手動実装)
static void LimitAndNormalizeWeights(std::vector<std::pair<int, float>>& weights)
{
	if (weights.size() > 4)
	{
		std::partial_sort(weights.begin(), weights.begin() + 4, weights.end(),
			[](const std::pair<int, float>& a, const std::pair<int, float>& b) { return a.second > b.second; });
		weights.resize(4);
	}

	float sum = 0.0f;
	for (auto& w : weights) sum += w.second;
	if (sum > 0.0f)
		for (auto& w : weights) w.second /= sum;
}


void FbxModelRenderer::LoadModel(const char *FileName, MODEL *Model, FbxSkinData** OutSkinData)
{
	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ioSettings = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ioSettings);

	FbxImporter* importer = FbxImporter::Create(manager, "");
	if (!importer->Initialize(FileName, -1, manager->GetIOSettings()))
	{
		MessageBoxA(nullptr, importer->GetStatus().GetErrorString(), "FBX Load Error", MB_OK);
		importer->Destroy();
		manager->Destroy();
		assert(false);
		return;
	}

	FbxScene* scene = FbxScene::Create(manager, "ModelScene");
	importer->Import(scene);
	importer->Destroy();

	// 全ポリゴンを三角形化(既存パイプラインは三角形リスト前提)
	FbxGeometryConverter geometryConverter(manager);
	geometryConverter.Triangulate(scene, true);

	// FBXの座標系(Maya等: 右手系が多い)からDirectXの左手座標系へ変換。
	// ConvertScene()はルートノードに補正回転を足すだけで、ハンドネス変更(右手系→左手系)を
	// 正しく表現できない(ドキュメントに明記)。DeepConvertScene()は階層全体の変換・頂点位置・
	// アニメーションカーブまで再計算するため、こちらを使う必要がある。
	FbxAxisSystem::DirectX.DeepConvertScene(scene);
	FbxSystemUnit::m.ConvertScene(scene);

	char dir[MAX_PATH];
	strcpy(dir, FileName);
	PathRemoveFileSpec(dir);

	std::vector<FbxNode*> meshNodes;
	CollectMeshNodesRecursive(scene->GetRootNode(), meshNodes);

	bool hasBones = false;
	for (FbxNode* node : meshNodes)
		if (node->GetMesh()->GetDeformerCount(FbxDeformer::eSkin) > 0)
			hasBones = true;

	//==================================================
	// スキニング情報(Skeleton)の構築。
	// メッシュ本体より先に、各ボーンのGPU行列インデックスを確定させておく。
	//==================================================
	Skeleton skeleton;
	if (hasBones)
	{
		skeleton.BuildFromScene(scene);

		int gpuIndex = 0;
		for (FbxNode* node : meshNodes)
		{
			FbxMesh* mesh = node->GetMesh();
			int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
			for (int sk = 0; sk < skinCount; sk++)
			{
				FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(sk, FbxDeformer::eSkin);
				int clusterCount = skin->GetClusterCount();
				for (int c = 0; c < clusterCount; c++)
				{
					FbxCluster* cluster = skin->GetCluster(c);
					FbxNode* link = cluster->GetLink();
					if (!link) continue;

					int boneIndex = skeleton.FindBoneIndex(link->GetName());
					if (boneIndex < 0) continue; // 通常は起こらないはず(ノード階層に必ず存在する)

					Bone& skelBone = skeleton.Bones[boneIndex];
					if (!skelBone.HasOffset)
					{
						FbxAMatrix transformMatrix;     // メッシュのバインド時グローバル変換
						FbxAMatrix transformLinkMatrix; // ボーンのバインド時グローバル変換
						cluster->GetTransformMatrix(transformMatrix);
						cluster->GetTransformLinkMatrix(transformLinkMatrix);

						FbxAMatrix offset = transformLinkMatrix.Inverse() * transformMatrix;

						skelBone.HasOffset    = true;
						skelBone.OffsetMatrix = ToXMMatrix(offset);
						skelBone.GpuIndex     = gpuIndex++;
					}
				}
			}
		}
		skeleton.GpuBoneCount = gpuIndex;
	}

	// 全メッシュを単一の頂点/インデックスバッファへ結合し、メッシュ単位でSUBSET(描画範囲+マテリアル)に分ける。
	unsigned int totalVertexNum = 0;
	for (FbxNode* node : meshNodes)
		totalVertexNum += node->GetMesh()->GetPolygonCount() * 3;
	unsigned int totalIndexNum = totalVertexNum; // 頂点を共有せず、ポリゴン頂点ごとに1頂点を割り当てる

	VERTEX_3D*      vertexArray     = hasBones ? nullptr : new VERTEX_3D[totalVertexNum];
	VERTEX_3D_SKIN* vertexArraySkin = hasBones ? new VERTEX_3D_SKIN[totalVertexNum] : nullptr;
	unsigned int* indexArray = new unsigned int[totalIndexNum];

	Model->SubsetArray = new SUBSET[meshNodes.size()];
	Model->SubsetNum = (unsigned int)meshNodes.size();

	unsigned int vertexOffset = 0;
	unsigned int indexOffset = 0;

	for (size_t m = 0; m < meshNodes.size(); m++)
	{
		FbxNode* node = meshNodes[m];
		FbxMesh* mesh = node->GetMesh();

		const int polygonCount = mesh->GetPolygonCount();
		const FbxVector4* controlPoints = mesh->GetControlPoints();

		//==============================
		// このメッシュのスキンウェイトを制御点ごとに集計(上位4件+正規化)
		//==============================
		std::vector<std::vector<std::pair<int, float>>> controlPointWeights;
		if (hasBones)
		{
			controlPointWeights.resize(mesh->GetControlPointsCount());

			int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
			for (int sk = 0; sk < skinCount; sk++)
			{
				FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(sk, FbxDeformer::eSkin);
				int clusterCount = skin->GetClusterCount();
				for (int c = 0; c < clusterCount; c++)
				{
					FbxCluster* cluster = skin->GetCluster(c);
					FbxNode* link = cluster->GetLink();
					if (!link) continue;

					int boneIndex = skeleton.FindBoneIndex(link->GetName());
					if (boneIndex < 0) continue;
					int gpuIndex = skeleton.Bones[boneIndex].GpuIndex;

					int indexCount = cluster->GetControlPointIndicesCount();
					int* indices = cluster->GetControlPointIndices();
					double* weights = cluster->GetControlPointWeights();

					for (int k = 0; k < indexCount; k++)
					{
						if (weights[k] <= 0.0) continue;
						controlPointWeights[indices[k]].push_back({ gpuIndex, (float)weights[k] });
					}
				}
			}

			for (auto& weights : controlPointWeights)
				LimitAndNormalizeWeights(weights);
		}

		FbxStringList uvSetNames;
		mesh->GetUVSetNames(uvSetNames);
		const char* uvSetName = (uvSetNames.GetCount() > 0) ? uvSetNames[0] : nullptr;

		unsigned int writeIndex = 0;
		for (int p = 0; p < polygonCount; p++)
		{
			if (mesh->GetPolygonSize(p) != 3) continue; // 三角形化済みのはずだが念のためガード

			for (int corner = 0; corner < 3; corner++)
			{
				int cpIndex = mesh->GetPolygonVertex(p, corner);
				FbxVector4 cp = controlPoints[cpIndex];

				FbxVector4 normal4;
				mesh->GetPolygonVertexNormal(p, corner, normal4);

				FbxVector2 uv2(0.0, 0.0);
				if (uvSetName)
				{
					bool unmapped = false;
					mesh->GetPolygonVertexUV(p, corner, uvSetName, uv2, unmapped);
				}

				XMFLOAT3 position = XMFLOAT3((float)cp[0], (float)cp[1], (float)cp[2]);
				XMFLOAT3 normal   = XMFLOAT3((float)normal4[0], (float)normal4[1], (float)normal4[2]);
				XMFLOAT2 texCoord = XMFLOAT2((float)uv2[0], 1.0f - (float)uv2[1]); // FBXのV軸はDirectXと上下逆

				unsigned int vid = vertexOffset + writeIndex;

				if (hasBones)
				{
					VERTEX_3D_SKIN& vertex = vertexArraySkin[vid];
					vertex.Position = position;
					vertex.Normal   = normal;
					vertex.TexCoord = texCoord;
					vertex.Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

					vertex.BoneIndices[0] = vertex.BoneIndices[1] = vertex.BoneIndices[2] = vertex.BoneIndices[3] = 0;
					vertex.BoneWeights = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

					const std::vector<std::pair<int, float>>& weights = controlPointWeights[cpIndex];
					if (weights.empty())
					{
						// どのボーンにも属さない頂点は、そのまま静止させるため単位行列(GpuIndex 0)へフルウェイトで逃がす
						vertex.BoneWeights = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
					}
					else
					{
						for (size_t w = 0; w < weights.size(); w++)
						{
							vertex.BoneIndices[w] = (UINT)weights[w].first;
							reinterpret_cast<float*>(&vertex.BoneWeights)[w] = weights[w].second;
						}
					}
				}
				else
				{
					VERTEX_3D& vertex = vertexArray[vid];
					vertex.Position = position;
					vertex.Normal   = normal;
					vertex.TexCoord = texCoord;
					vertex.Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				}

				indexArray[indexOffset + writeIndex] = vid;
				writeIndex++;
			}
		}

		SUBSET& subset = Model->SubsetArray[m];
		subset.StartIndex = indexOffset;
		subset.IndexNum = writeIndex;
		subset.Material.Texture = nullptr;
		strncpy(subset.Material.Name, node->GetName(), sizeof(subset.Material.Name) - 1);
		subset.Material.Name[sizeof(subset.Material.Name) - 1] = '\0';

		//==============================
		// マテリアル読み込み
		//==============================
		FbxSurfaceMaterial* material = (node->GetMaterialCount() > 0) ? node->GetMaterial(0) : nullptr;

		FbxDouble3 diffuse(1.0, 1.0, 1.0);
		FbxDouble3 ambient(0.2, 0.2, 0.2);
		FbxDouble3 specular(0.0, 0.0, 0.0);
		double shininess = 0.0;

		if (material)
		{
			FbxProperty prop;
			prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			if (prop.IsValid()) diffuse = prop.Get<FbxDouble3>();
			prop = material->FindProperty(FbxSurfaceMaterial::sAmbient);
			if (prop.IsValid()) ambient = prop.Get<FbxDouble3>();
			prop = material->FindProperty(FbxSurfaceMaterial::sSpecular);
			if (prop.IsValid()) specular = prop.Get<FbxDouble3>();
			prop = material->FindProperty(FbxSurfaceMaterial::sShininess);
			if (prop.IsValid()) shininess = prop.Get<FbxDouble>();
			// sTransparencyFactorは単体では信頼できない(多くのエクスポータがTransparentColorとの
			// 掛け算前提で1.0を既定値にしており、そのままアルファに使うと不透明マテリアルが透明になる)。
			// 今回のFBXは透明マテリアルを使わないため、アルファは常に不透明(1.0)とする。
		}

		subset.Material.Material.Diffuse   = XMFLOAT4((float)diffuse[0], (float)diffuse[1], (float)diffuse[2], 1.0f);
		subset.Material.Material.Ambient   = XMFLOAT4((float)ambient[0], (float)ambient[1], (float)ambient[2], 1.0f);
		subset.Material.Material.Specular  = XMFLOAT4((float)specular[0], (float)specular[1], (float)specular[2], 1.0f);
		subset.Material.Material.Emission  = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		subset.Material.Material.Shininess = (float)shininess;

		//==============================
		// テクスチャ読み込み(外部ファイル参照)
		//==============================
		if (material)
		{
			FbxProperty diffuseProp = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			int fileTexCount = diffuseProp.IsValid() ? diffuseProp.GetSrcObjectCount<FbxFileTexture>() : 0;
			if (fileTexCount > 0)
			{
				FbxFileTexture* tex = diffuseProp.GetSrcObject<FbxFileTexture>(0);
				const char* texFileName = tex->GetRelativeFileName();
				if (!texFileName || texFileName[0] == '\0')
					texFileName = tex->GetFileName();

				if (texFileName && texFileName[0] != '\0')
				{
					char path[MAX_PATH];
					strcpy(path, dir);
					strcat(path, "\\");
					strcat(path, PathFindFileNameA(texFileName));

					wchar_t wpath[MAX_PATH];
					mbstowcs(wpath, path, MAX_PATH);

					TexMetadata metadata;
					ScratchImage image;
					if (SUCCEEDED(LoadFromWICFile(wpath, WIC_FLAGS_NONE, &metadata, image)))
					{
						ScratchImage mipChain;
						if (SUCCEEDED(GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain)))
							CreateShaderResourceView(Renderer::GetDevice(), mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), &subset.Material.Texture);
						else
							CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &subset.Material.Texture);
					}
				}
			}
		}

		subset.Material.Material.TextureEnable = (subset.Material.Texture != nullptr);

		vertexOffset += writeIndex;
		indexOffset += writeIndex;
	}

	//==============================
	// 頂点バッファ生成
	//==============================
	{
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = hasBones ? (sizeof(VERTEX_3D_SKIN) * vertexOffset) : (sizeof(VERTEX_3D) * vertexOffset);
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
		bd.ByteWidth = sizeof(unsigned int) * indexOffset;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd{};
		sd.pSysMem = indexArray;

		Renderer::GetDevice()->CreateBuffer(&bd, &sd, &Model->IndexBuffer);
	}

	delete[] vertexArray;
	delete[] vertexArraySkin;
	delete[] indexArray;

	// Skeleton(骨格の共有データ)はここで確定させる。
	// GPUへ送るボーン行列バッファはインスタンスごとに再生状態が異なりうるため、Load()側で生成する。
	if (hasBones)
	{
		FbxSkinData* skinData = new FbxSkinData();
		skinData->Skeleton = skeleton;
		*OutSkinData = skinData;
	}

	scene->Destroy();
	manager->Destroy();
}
