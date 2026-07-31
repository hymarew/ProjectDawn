#include "main.h"
#include "fbxTestObject.h"
#include "fbxModelRenderer.h"

ID3D11VertexShader* FbxTestObject::m_VertexShader = nullptr;
ID3D11PixelShader*  FbxTestObject::m_PixelShader  = nullptr;
ID3D11InputLayout*  FbxTestObject::m_VertexLayout = nullptr;

ID3D11VertexShader* FbxTestObject::m_SkinVertexShader = nullptr;
ID3D11InputLayout*  FbxTestObject::m_SkinVertexLayout = nullptr;

bool                FbxTestObject::m_IsLoaded     = false;

void FbxTestObject::Init()
{
	m_Layer    = 1;
	m_Position = { 0.0f, 0.0f, 0.0f };

	// MixamoのFBXは実寸(cm相当)で書き出されているため、ゲーム内スケールに合わせて0.01倍する
	m_Scale = { 0.01f, 0.01f, 0.01f };

	// Idle.fbxはメッシュ・スキン・Idleアニメーションを1本にまとめて含んでいるため、
	// メッシュ読み込みと再生アニメーションの両方をこのファイルから取る
	m_ModelRenderer = AddComponent<FbxModelRenderer>(this);
	m_ModelRenderer->Load("asset\\model\\fbx\\Idle.fbx");
	m_ModelRenderer->PlayAnimation("asset\\model\\fbx\\Idle.fbx");

	if (!m_IsLoaded)
	{
		Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
			"shader\\ShadowMapLightingVS.cso");
		Renderer::CreatePixelShader(&m_PixelShader,
			"shader\\ShadowMapLightingPS.cso");

		Renderer::CreateVertexShaderSkinned(&m_SkinVertexShader, &m_SkinVertexLayout,
			"shader\\SkinningVS.cso");

		m_IsLoaded = true;
	}

	Light.Enable     = TRUE;
	Light.CastShadow = FALSE; // Phase1/2はシャドウマップ未セットアップのテストシーンのため無効化
	Light.Direction  = XMFLOAT4(0.3f, -0.7f, 0.5f, 0.0f);
	Light.Diffuse    = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Light.Ambient    = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
}

bool FbxTestObject::IsSkinned() const
{
	return m_ModelRenderer && m_ModelRenderer->IsSkinned();
}

void FbxTestObject::PlayAnimation(const char* fileName)
{
	if (m_ModelRenderer)
		m_ModelRenderer->PlayAnimation(fileName);
}

bool FbxTestObject::GetBoneWorldPositions(std::vector<Vector3>& outPositions, std::vector<int>& outParentIndices, std::vector<bool>& outHasOffset) const
{
	if (!m_ModelRenderer) return false;

	std::vector<XMFLOAT3> localPositions;
	if (!m_ModelRenderer->GetCurrentBonePositions(localPositions, outParentIndices, outHasOffset))
		return false;

	XMMATRIX scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	XMMATRIX rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	XMMATRIX trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	XMMATRIX world = scale * rot * trans;

	outPositions.resize(localPositions.size());
	for (size_t i = 0; i < localPositions.size(); i++)
	{
		XMVECTOR worldPos = XMVector3Transform(XMLoadFloat3(&localPositions[i]), world);
		outPositions[i] = Vector3(XMVectorGetX(worldPos), XMVectorGetY(worldPos), XMVectorGetZ(worldPos));
	}

	return true;
}

bool FbxTestObject::HasAnimation() const
{
	return m_ModelRenderer && m_ModelRenderer->HasAnimation();
}

const char* FbxTestObject::GetCurrentClipName() const
{
	return m_ModelRenderer ? m_ModelRenderer->GetCurrentClipName() : "";
}

float FbxTestObject::GetAnimTime() const
{
	return m_ModelRenderer ? m_ModelRenderer->GetAnimTime() : 0.0f;
}

float FbxTestObject::GetClipDuration() const
{
	return m_ModelRenderer ? m_ModelRenderer->GetClipDuration() : 0.0f;
}

int FbxTestObject::GetSkeletonBoneCount() const
{
	return m_ModelRenderer ? m_ModelRenderer->GetSkeletonBoneCount() : 0;
}

int FbxTestObject::GetMatchedAnimationBoneCount() const
{
	return m_ModelRenderer ? m_ModelRenderer->GetMatchedAnimationBoneCount() : 0;
}

void FbxTestObject::Update(float dt)
{
	GameObject::Update(dt);
}

void FbxTestObject::Draw()
{
	bool skinned = m_ModelRenderer && m_ModelRenderer->IsSkinned();

	Renderer::GetDeviceContext()->IASetInputLayout(skinned ? m_SkinVertexLayout : m_VertexLayout);

	Renderer::SetLight(Light);

	Renderer::GetDeviceContext()->VSSetShader(skinned ? m_SkinVertexShader : m_VertexShader, NULL, 0);
	Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

	XMMATRIX world, scale, rot, trans;
	scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
	rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	world = scale * rot * trans;

	Renderer::SetWorldMatrix(world);
	GameObject::Draw();
}
