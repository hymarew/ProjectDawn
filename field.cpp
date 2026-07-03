#include "main.h"
#include "renderer.h"
#include "field.h"
#include "DirectXTex.h"

void Field::Init()
{
    m_Layer = 1;
    VERTEX_3D vertex[4];
    {
        vertex[0].Position = XMFLOAT3(-300.0f, 0.0f, 300.0f);
        vertex[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

        vertex[1].Position = XMFLOAT3(300.0f, 0.0f, 300.0f);
        vertex[1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[1].TexCoord = XMFLOAT2(10.0f, 0.0f);

        vertex[2].Position = XMFLOAT3(-300.0f, 0.0f, -300.0f);
        vertex[2].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[2].TexCoord = XMFLOAT2(0.0f, 10.0f);

        vertex[3].Position = XMFLOAT3(300.0f, 0.0f, -300.0f);
        vertex[3].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex[3].TexCoord = XMFLOAT2(10.0f, 10.0f);
    }

    // 頂点バッファ生成
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(VERTEX_3D) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;

    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

    //地面に影を入れるためにShadowMapShaderを
    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\ShadowMapLightingVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\ShadowMapLightingPS.cso");

    // テクスチャ読み込み（アセットが無ければテクスチャ無しで続行する）
    m_Texture = nullptr;
    TexMetadata metadata;
    ScratchImage image;
    if (SUCCEEDED(LoadFromWICFile(L"asset\\model\\grass.jpg", WIC_FLAGS_NONE, &metadata, image)))
    {
        CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
            image.GetImageCount(), metadata, &m_Texture);
    }
}

void Field::Uninit()
{
    //セーフリリースにしてもいい
    m_VertexBuffer->Release();

    m_VertexLayout->Release();
    m_VertexShader->Release();
    m_PixelShader->Release();

    if (m_Texture) m_Texture->Release();
}

void Field::Update(float dt)
{

}

void Field::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    //マトリクス設定
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    // マテリアル設定
    MATERIAL material{};
    material.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
    material.TextureEnable = (m_Texture != nullptr);
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

void Field::DrawShadow()
{
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

    // 4つの頂点を描画する（Draw命令）
    Renderer::GetDeviceContext()->Draw(4, 0);
}