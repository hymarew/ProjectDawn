#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "explosion.h"
#include "DirectXTex.h"
#include "camera.h"
#include "GameConfig.h"

ID3D11ShaderResourceView* Explosion::m_Texture = nullptr;
ID3D11VertexShader* Explosion::m_VertexShader = nullptr;
ID3D11PixelShader* Explosion::m_PixelShader = nullptr;
ID3D11InputLayout* Explosion::m_VertexLayout = nullptr;

void Explosion::Init()
{
    m_Layer = 2;
    VERTEX_3D vertex[4];
    {
        vertex[0].Position = XMFLOAT3(-1.0f, 1.0f, 0.0f);
        vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

        vertex[1].Position = XMFLOAT3(1.0f, 1.0f, 0.0f);
        vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

        vertex[2].Position = XMFLOAT3(-1.0f, 0.0f, 0.0f);
        vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

        vertex[3].Position = XMFLOAT3(1.0f, 0.0f, 0.0f);
        vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);
    }

    // 頂点バッファ生成
    // --- 頂点バッファを「動的(DYNAMIC)」に変更し、毎フレームの書き込みを許可する ---
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX_3D) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;

    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

    // テクスチャとシェーダーは、まだ読み込まれていない最初の1回だけロードする
    if (m_Texture == nullptr)
    {
        //地面に影を入れるためにShadowMapShaderを
        // シェーダー読込
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\ShadowMapLightingVS.cso");

        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\ShadowMapLightingPS.cso");

        // テクスチャ読み込み
        TexMetadata metadata;
        ScratchImage image;
        LoadFromWICFile(L"asset\\texture\\Explosion.png", WIC_FLAGS_NONE, &metadata, image);
        CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
            image.GetImageCount(), metadata, &m_Texture);

        assert(m_Texture);
    }
    m_Frame = 0;

    m_Scale = { 4.0f,4.0f ,4.0f };
}

void Explosion::Uninit()
{
    //セーフリリースにしてもいい
    m_VertexBuffer->Release();
}

void Explosion::Update(float dt)
{
    m_Frame++;

    if (m_Frame >= GameConfig::Explosion::ANIM_FRAME_MAX)
    {
        SetDestroy();
    }
}

void Explosion::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    //billboard設定

        D3D11_MAPPED_SUBRESOURCE msr;
        Renderer::GetDeviceContext()->Map(m_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

        VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

        float tx = 1.0f / GameConfig::Explosion::ANIM_SHEET_COLS * (m_Frame % (int)GameConfig::Explosion::ANIM_SHEET_COLS);
        float ty = 1.0f / GameConfig::Explosion::ANIM_SHEET_ROWS * (m_Frame / (int)GameConfig::Explosion::ANIM_SHEET_COLS);
        float tw = 1.0f / GameConfig::Explosion::ANIM_SHEET_COLS;
        float th = 1.0f / GameConfig::Explosion::ANIM_SHEET_ROWS;

        vertex[0].Position = XMFLOAT3(-1.0f, 1.0f, 0.0f);
        vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(tx, ty);

        vertex[1].Position = XMFLOAT3(1.0f, 1.0f, 0.0f);
        vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(tx + tw, ty);

        vertex[2].Position = XMFLOAT3(-1.0f, -1.0f, 0.0f);
        vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(tx, ty + th);

        vertex[3].Position = XMFLOAT3(1.0f, -1.0f, 0.0f);
        vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[3].TexCoord = XMFLOAT2(tx + tw, ty + th);

        Renderer::GetDeviceContext()->Unmap(m_VertexBuffer, 0);

        Camera* camera = Manager::GetCamera();    XMMATRIX view = camera->GetViewMatrix();
        XMMATRIX invView = XMMatrixInverse(NULL, view); //逆行列
        invView.r[3].m128_f32[0] = 0.0f;    //動かないように設定
        invView.r[3].m128_f32[1] = 0.0f;
        invView.r[3].m128_f32[2] = 0.0f;
    

    //マトリクス設定
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    //rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * invView * trans;

    Renderer::SetWorldMatrix(world);

    // マテリアル設定
    MATERIAL material{};
    material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    //テクスチャ設定
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);

    // 頂点バッファ設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);

    // プリミティブトポロジ設定
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // ポリゴン描画
    Renderer::GetDeviceContext()->Draw(4, 0);
}

void Explosion::DrawShadow()
{
    return;
    // ==================================================
    // 1. 頂点のレイアウト設定
    // ==================================================
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // ==================================================
    // 2. 自分の「位置・回転・大きさ」からワールド行列を計算してGPUに伝える
    // ==================================================
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    // ==================================================
    // 3. 自分で管理している頂点バッファをセットして描画命令を出す
    // ==================================================
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    // 頂点バッファをセット
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    // 三角形のストリップ（帯状）として描画することを伝える
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);

    // 4つの頂点を描画する（Draw命令）
    Renderer::GetDeviceContext()->Draw(4, 0);
}