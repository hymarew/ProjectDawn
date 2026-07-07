#include "main.h"
#include "tree.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "sphereCollider.h"
#include "collisionManager.h"
#include "camera.h"
#include "GameConfig.h"
#include <cstring>

//--------------------------------------------------------------------
// 静的メンバ定義
//--------------------------------------------------------------------
ID3D11InputLayout*     Tree::m_VertexLayout     = nullptr;
ID3D11VertexShader*    Tree::m_VertexShader     = nullptr;
ID3D11PixelShader*     Tree::m_PixelShader      = nullptr;
ID3D11InputLayout*     Tree::m_LeafVertexLayout = nullptr;
ID3D11VertexShader*    Tree::m_LeafVertexShader = nullptr;
ID3D11PixelShader*     Tree::m_LeafPixelShader  = nullptr;
ID3D11Buffer*          Tree::m_WindBuffer       = nullptr;
ID3D11RasterizerState* Tree::m_RsCullBack       = nullptr;
ID3D11RasterizerState* Tree::m_RsCullNone       = nullptr;
bool                   Tree::m_IsLoaded         = false;
TreeModel              Tree::m_TreeModels[(int)TreeType::Count] = {};

// モデルパステーブル [TreeType][LOD0-2]
static const char* TREE_MODEL_PATHS[(int)TreeType::Count][3] =
{
    { "asset\\model\\Tree\\Tree_05_A_LOD0.obj", "asset\\model\\Tree\\Tree_05_A_LOD1.obj", "asset\\model\\Tree\\Tree_05_A_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_05_B_LOD0.obj", "asset\\model\\Tree\\Tree_05_B_LOD1.obj", "asset\\model\\Tree\\Tree_05_B_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_05_C_LOD0.obj", "asset\\model\\Tree\\Tree_05_C_LOD1.obj", "asset\\model\\Tree\\Tree_05_C_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_08_A_LOD0.obj", "asset\\model\\Tree\\Tree_08_A_LOD1.obj", "asset\\model\\Tree\\Tree_08_A_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_08_B_LOD0.obj", "asset\\model\\Tree\\Tree_08_B_LOD1.obj", "asset\\model\\Tree\\Tree_08_B_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_08_C_LOD0.obj", "asset\\model\\Tree\\Tree_08_C_LOD1.obj", "asset\\model\\Tree\\Tree_08_C_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_10_A_LOD0.obj", "asset\\model\\Tree\\Tree_10_A_LOD1.obj", "asset\\model\\Tree\\Tree_10_A_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_10_B_LOD0.obj", "asset\\model\\Tree\\Tree_10_B_LOD1.obj", "asset\\model\\Tree\\Tree_10_B_LOD2.obj" },
    { "asset\\model\\Tree\\Tree_10_C_LOD0.obj", "asset\\model\\Tree\\Tree_10_C_LOD1.obj", "asset\\model\\Tree\\Tree_10_C_LOD2.obj" },
};

// LOD切り替え距離（カメラ距離の二乗）
static constexpr float LOD1_DIST_SQ = 30.0f * 30.0f;
static constexpr float LOD2_DIST_SQ = 60.0f * 60.0f;

// 風アニメーション用（全インスタンス共有の時間・インスタンス数）
static float s_WindTime  = 0.0f;
static int   s_TreeCount = 0;

// 風定数バッファのデータ構造（HLSL WindBuffer と一致）
struct WindCB
{
    float Time;
    float Strength;
    float Dummy[2];
};

//--------------------------------------------------------------------
void Tree::Init()
{
    m_Layer = 2;
    s_TreeCount++;

    if (!m_IsLoaded)
    {
        // 幹用シェーダー（ShadowMapLighting）
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\ShadowMapLightingPS.cso");

        // 葉用シェーダー（風アニメーション + Alpha Clip）
        Renderer::CreateVertexShader(&m_LeafVertexShader, &m_LeafVertexLayout,
            "shader\\TreeLeafVS.cso");
        Renderer::CreatePixelShader(&m_LeafPixelShader,
            "shader\\TreeLeafPS.cso");

        // 風アニメーション用定数バッファ（register b7）
        D3D11_BUFFER_DESC windDesc{};
        windDesc.ByteWidth      = sizeof(WindCB);
        windDesc.Usage          = D3D11_USAGE_DEFAULT;
        windDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        Renderer::GetDevice()->CreateBuffer(&windDesc, nullptr, &m_WindBuffer);

        // ラスタライザステート（幹: CullBack / 葉: CullNone）
        ID3D11Device* dev = Renderer::GetDevice();
        D3D11_RASTERIZER_DESC rsDesc{};
        rsDesc.FillMode        = D3D11_FILL_SOLID;
        rsDesc.DepthClipEnable = TRUE;

        rsDesc.CullMode = D3D11_CULL_BACK;
        dev->CreateRasterizerState(&rsDesc, &m_RsCullBack);

        rsDesc.CullMode = D3D11_CULL_NONE;
        dev->CreateRasterizerState(&rsDesc, &m_RsCullNone);

        // 全TreeType × 全LODをプリロード
        for (int t = 0; t < (int)TreeType::Count; t++)
            for (int lv = 0; lv < 3; lv++)
            {
                ModelRenderer::Preload(TREE_MODEL_PATHS[t][lv]);
                m_TreeModels[t].lod[lv] = ModelRenderer::GetCachedModel(TREE_MODEL_PATHS[t][lv]);
            }

        m_IsLoaded = true;
    }

    // 幹コリジョン（Capsule相当としてSphereColliderで近似）
    auto* col = AddComponent<SphereCollider>(this);
    col->Setup(GameConfig::Collision::TREE_RADIUS, ColliderTag::Obstacle);
    g_CollisionManager.Register(col);
}

//--------------------------------------------------------------------
void Tree::Uninit()
{
    if (s_TreeCount > 0) s_TreeCount--;

    SphereCollider* col = GetComponent<SphereCollider>();
    if (col) g_CollisionManager.Unregister(col);
    GameObject::Uninit();
}

//--------------------------------------------------------------------
void Tree::Update(float dt)
{
    // 全インスタンスで1フレームあたり dt だけ進むよう分担
    if (s_TreeCount > 0)
        s_WindTime += dt / (float)s_TreeCount;

    GameObject::Update(dt);
}

//--------------------------------------------------------------------
// 通常描画（幹: 既存シェーダー / 葉: 風アニメ＋Alpha Clipシェーダー）
//--------------------------------------------------------------------
void Tree::Draw()
{
    if (m_IsDestroyed) return;

    Camera* camera = Manager::GetCamera();
    if (camera && !camera->CheckInView(m_Position)) return;

    float distSq = 0.0f;
    if (camera)
    {
        Vector3 camPos = camera->GetPosition();
        float dx = m_Position.x - camPos.x;
        float dy = m_Position.y - camPos.y;
        float dz = m_Position.z - camPos.z;
        distSq = dx * dx + dy * dy + dz * dz;
    }

    MODEL* model = m_TreeModels[(int)m_TreeType].lod[SelectLOD(distSq)];
    if (!model) return;

    // 風定数バッファをVSのb7へセット（葉シェーダーが参照）
    WindCB wdata = { s_WindTime, 0.1f, { 0.0f, 0.0f } };
    Renderer::GetDeviceContext()->UpdateSubresource(m_WindBuffer, 0, nullptr, &wdata, 0, 0);
    Renderer::GetDeviceContext()->VSSetConstantBuffers(7, 1, &m_WindBuffer);

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;

    for (unsigned int i = 0; i < model->SubsetNum; i++)
    {
        const SUBSET& sub = model->SubsetArray[i];
        bool leaf = IsLeafMaterial(sub.Material.Name);

        if (leaf)
        {
            // 葉: 風アニメーション VS + Alpha Clip PS + 両面描画
            Renderer::GetDeviceContext()->IASetInputLayout(m_LeafVertexLayout);
            Renderer::GetDeviceContext()->VSSetShader(m_LeafVertexShader, nullptr, 0);
            Renderer::GetDeviceContext()->PSSetShader(m_LeafPixelShader,  nullptr, 0);
            Renderer::GetDeviceContext()->RSSetState(m_RsCullNone);
        }
        else
        {
            // 幹: 通常シェーダー + BackFace Culling
            Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
            Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
            Renderer::GetDeviceContext()->PSSetShader(m_PixelShader,  nullptr, 0);
            Renderer::GetDeviceContext()->RSSetState(m_RsCullBack);
        }

        Renderer::SetMaterial(sub.Material.Material);
        if (sub.Material.Texture)
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &sub.Material.Texture);

        Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer, &stride, &offset);
        Renderer::GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Renderer::GetDeviceContext()->DrawIndexed(sub.IndexNum, sub.StartIndex, 0);
    }

    // 後続オブジェクトのために既定状態に戻す
    Renderer::GetDeviceContext()->RSSetState(m_RsCullBack);
}

//--------------------------------------------------------------------
// シャドウ描画（Alpha Clip のためテクスチャをバインド）
//--------------------------------------------------------------------
void Tree::DrawShadow()
{
    if (m_IsDestroyed) return;

    MODEL* model = m_TreeModels[(int)m_TreeType].lod[SelectLOD(0.0f)];
    if (!model) return;

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z)
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;

    for (unsigned int i = 0; i < model->SubsetNum; i++)
    {
        const SUBSET& sub = model->SubsetArray[i];

        if (sub.Material.Texture)
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &sub.Material.Texture);

        bool leaf = IsLeafMaterial(sub.Material.Name);
        Renderer::GetDeviceContext()->RSSetState(leaf ? m_RsCullNone : m_RsCullBack);

        Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer, &stride, &offset);
        Renderer::GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Renderer::GetDeviceContext()->DrawIndexed(sub.IndexNum, sub.StartIndex, 0);
    }

    Renderer::GetDeviceContext()->RSSetState(m_RsCullBack);
}

//--------------------------------------------------------------------
bool Tree::IsLeafMaterial(const char* name) const
{
    return strstr(name, "Leaf") != nullptr || strstr(name, "leaf") != nullptr;
}

//--------------------------------------------------------------------
int Tree::SelectLOD(float cameraDistSq) const
{
    const TreeModel& tm = m_TreeModels[(int)m_TreeType];
    if (cameraDistSq > LOD2_DIST_SQ && tm.lod[2]) return 2;
    if (cameraDistSq > LOD1_DIST_SQ && tm.lod[1]) return 1;
    return 0;
}

//--------------------------------------------------------------------
void Tree::UpdateWind(float time)
{
    s_WindTime = time;
}

void Tree::TakeDamage(float dmg)
{
    (void)dmg;
}

//--------------------------------------------------------------------
void Tree::ReleaseShaders()
{
    if (m_VertexShader)     { m_VertexShader    ->Release(); m_VertexShader     = nullptr; }
    if (m_PixelShader)      { m_PixelShader     ->Release(); m_PixelShader      = nullptr; }
    if (m_VertexLayout)     { m_VertexLayout    ->Release(); m_VertexLayout     = nullptr; }
    if (m_LeafVertexShader) { m_LeafVertexShader->Release(); m_LeafVertexShader = nullptr; }
    if (m_LeafPixelShader)  { m_LeafPixelShader ->Release(); m_LeafPixelShader  = nullptr; }
    if (m_LeafVertexLayout) { m_LeafVertexLayout->Release(); m_LeafVertexLayout = nullptr; }
    if (m_WindBuffer)       { m_WindBuffer      ->Release(); m_WindBuffer       = nullptr; }
    if (m_RsCullBack)       { m_RsCullBack      ->Release(); m_RsCullBack       = nullptr; }
    if (m_RsCullNone)       { m_RsCullNone      ->Release(); m_RsCullNone       = nullptr; }
    m_IsLoaded = false;
}
