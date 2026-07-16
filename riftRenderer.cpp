#include "main.h"
#include "riftRenderer.h"
#include "renderer.h"

void RiftRenderer::Init()
{
    // 1×1のQuad(中心原点、法線+Z)。本体・地面投影ともにこのジオメトリをTransformの
    // 回転・スケールで使い分ける
    VERTEX_3D vertex[4];
    vertex[0] = { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT3(0,0,1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
    vertex[1] = { XMFLOAT3( 0.5f,  0.5f, 0.0f), XMFLOAT3(0,0,1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
    vertex[2] = { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT3(0,0,1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
    vertex[3] = { XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT3(0,0,1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(VERTEX_3D) * 4;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;
    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

    // 既存のSpotLightingVS(WorldPosition/Normalを埋める汎用VS)を流用する
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\SpotLightingVS.cso");
    Renderer::CreatePixelShader(&m_RiftPS,   "shader\\riftPS.cso");
    Renderer::CreatePixelShader(&m_GroundPS, "shader\\riftGroundPS.cso");

    // 板ポリゴンなので、回転によって背面が向いても消えないよう両面描画にする
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode        = D3D11_FILL_SOLID;
    rd.CullMode        = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    Renderer::GetDevice()->CreateRasterizerState(&rd, &m_NoCullState);
}

void RiftRenderer::Uninit()
{
    if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
    if (m_VertexLayout) { m_VertexLayout->Release(); m_VertexLayout = nullptr; }
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_RiftPS)       { m_RiftPS->Release();       m_RiftPS       = nullptr; }
    if (m_GroundPS)     { m_GroundPS->Release();     m_GroundPS     = nullptr; }
    if (m_NoCullState)  { m_NoCullState->Release();  m_NoCullState  = nullptr; }
}

namespace
{
    // 描画直前に共通で行うジオメトリ・カリング設定。
    // 呼び出し前後でRasterizerStateを退避/復元し、既存の描画パイプラインに影響を与えない
    struct ScopedNoCull
    {
        ID3D11DeviceContext*   ctx;
        ID3D11RasterizerState* prev = nullptr;

        explicit ScopedNoCull(ID3D11DeviceContext* context, ID3D11RasterizerState* noCull) : ctx(context)
        {
            ctx->RSGetState(&prev);
            ctx->RSSetState(noCull);
        }
        ~ScopedNoCull()
        {
            ctx->RSSetState(prev);
            if (prev) prev->Release();
        }
    };
}

void RiftRenderer::Draw(const Transform& transform, const RiftMaterial& material,
                        ID3D11ShaderResourceView* crackMaskSRV,
                        ID3D11ShaderResourceView* backgroundSRV) const
{
    // ガラス本体は通常のAlphaブレンドで、背景キャプチャ(t1)も使う
    ID3D11ShaderResourceView* srvs[2] = { crackMaskSRV, backgroundSRV };
    DrawInternal(transform, material, m_RiftPS, srvs, 2, false);
}

void RiftRenderer::DrawGround(const Transform& transform, const RiftMaterial& material,
                              ID3D11ShaderResourceView* crackMaskSRV) const
{
    // 地面投影はAlphaを使わず常に加算合成
    DrawInternal(transform, material, m_GroundPS, &crackMaskSRV, 1, true);
}

// ---------------------------------------------------------
// DrawInternal : 本体・地面投影で共通のドロー処理
// ---------------------------------------------------------
void RiftRenderer::DrawInternal(const Transform& transform, const RiftMaterial& material,
                                ID3D11PixelShader* pixelShader,
                                ID3D11ShaderResourceView* const* srvs, UINT srvCount,
                                bool additiveBlend) const
{
    auto* ctx = Renderer::GetDeviceContext();
    ScopedNoCull noCull(ctx, m_NoCullState);

    ctx->IASetInputLayout(m_VertexLayout);
    ctx->VSSetShader(m_VertexShader, nullptr, 0);
    ctx->PSSetShader(pixelShader, nullptr, 0);

    Renderer::SetWorldMatrix(transform.GetWorldMatrix());

    MATERIAL mat{};
    mat.Diffuse       = XMFLOAT4(1, 1, 1, 1);
    mat.TextureEnable = true;
    Renderer::SetMaterial(mat);

    material.Bind();

    ctx->PSSetShaderResources(0, srvCount, srvs);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Renderer::SetAdditiveBlend(additiveBlend);
    ctx->Draw(4, 0);
    Renderer::SetAdditiveBlend(false); // 通常のAlphaブレンドへ戻す（描画パイプラインの既定状態）
}
