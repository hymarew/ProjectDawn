#include "main.h"
#include "renderer.h"
#include "fullScreenQuad.h"
#include "DirectXTex.h"

// ---------------------------------------------------------
// Init : 画面全体を覆う矩形の頂点バッファと、1×1白テクスチャを用意する
// ---------------------------------------------------------
void FullScreenQuad::Init()
{
    // SetWorldViewProjection2D() のピクセル座標系（左上(0,0)〜右下(SCREEN_WIDTH,SCREEN_HEIGHT)）に合わせる
    VERTEX_3D vertex[4];
    vertex[0] = { XMFLOAT3(0.0f,                0.0f,                 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
    vertex[1] = { XMFLOAT3((float)SCREEN_WIDTH, 0.0f,                 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
    vertex[2] = { XMFLOAT3(0.0f,                (float)SCREEN_HEIGHT, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
    vertex[3] = { XMFLOAT3((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(VERTEX_3D) * 4;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;
    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

    // 既存のunlitテクスチャシェーダーを流用（Sky/パーティクルと同じもの）
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso");
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\unlitTexturePS.cso");

    // ---- 1×1 白テクスチャを読み込む ----
    TexMetadata  metadata;
    ScratchImage image;
    LoadFromWICFile(L"asset\\texture\\white.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
        image.GetImageCount(), metadata, &m_WhiteTexture);

    assert(m_WhiteTexture);
}

// ---------------------------------------------------------
// Uninit : GPUリソースの解放
// ---------------------------------------------------------
void FullScreenQuad::Uninit()
{
    if (m_WhiteTexture) { m_WhiteTexture->Release(); m_WhiteTexture = nullptr; }
    if (m_PixelShader)  { m_PixelShader->Release();  m_PixelShader  = nullptr; }
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_VertexLayout) { m_VertexLayout->Release();  m_VertexLayout = nullptr; }
    if (m_VertexBuffer) { m_VertexBuffer->Release();  m_VertexBuffer = nullptr; }
}

// ---------------------------------------------------------
// Draw : 画面全体を color で塗りつぶす（アルファ0以下なら何もしない）
// ---------------------------------------------------------
void FullScreenQuad::Draw(const XMFLOAT4& color, const XMMATRIX& world) const
{
    if (color.w <= 0.0f) return;

    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader,  nullptr, 0);

    Renderer::SetWorldViewProjection2D();
    Renderer::SetWorldMatrix(world);

    MATERIAL material{};
    material.Diffuse       = color;
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_WhiteTexture);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 深度を無視して常に最前面に描く
    Renderer::SetDepthEnable(false);
    Renderer::GetDeviceContext()->Draw(4, 0);
    Renderer::SetDepthEnable(true);
}

// ---------------------------------------------------------
// DrawGeometry : 頂点シェーダーとジオメトリだけ設定して描画する。
// ピクセルシェーダー・定数バッファ・テクスチャは呼び出し側が事前に設定しておくこと。
// ---------------------------------------------------------
void FullScreenQuad::DrawGeometry() const
{
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);

    Renderer::SetWorldViewProjection2D();
    Renderer::SetWorldMatrix(XMMatrixIdentity());

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Renderer::SetDepthEnable(false);
    Renderer::GetDeviceContext()->Draw(4, 0);
    Renderer::SetDepthEnable(true);
}
