#include "main.h"
#include "weatherRenderer.h"
#include "renderer.h"
#include "DirectXTex.h"
#include <cassert>
#include <cstdio>
#include <io.h>

namespace
{
    // 共有板ポリの頂点（位置とUVだけの最小構成。particleRenderer と同じ）
    struct QuadVertex
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
    };

    // shader\weatherParams.hlsl の WeatherParams (register b10) とレイアウトを一致させること
    struct WeatherParamsCB
    {
        float FadeStart = 0.0f;
        float FadeEnd   = 0.0f;
        float UVStretch = 1.0f;
        float _Pad      = 0.0f;
    };
}

void WeatherRenderer::Init(int maxInstances)
{
    m_MaxInstances = maxInstances;

    // ---- 板ポリゴンの頂点データ（中心原点の1×1。全粒がこれを共有する） ----
    QuadVertex vertex[4];
    vertex[0] = { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT2(0, 0) };
    vertex[1] = { XMFLOAT3( 0.5f,  0.5f, 0.0f), XMFLOAT2(1, 0) };
    vertex[2] = { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0, 1) };
    vertex[3] = { XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT2(1, 1) };

    D3D11_BUFFER_DESC bd{};
    bd.Usage     = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(QuadVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;
    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_QuadVertexBuffer);

    // ---- インスタンスバッファ（毎フレームCPUから書き込むためDYNAMIC） ----
    // 天候は最大数が固定（雨10000+雪5000）のため、拡張機構は持たず一括確保する
    D3D11_BUFFER_DESC ibd{};
    ibd.Usage          = D3D11_USAGE_DYNAMIC;
    ibd.ByteWidth      = sizeof(WeatherInstance) * maxInstances;
    ibd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::GetDevice()->CreateBuffer(&ibd, nullptr, &m_InstanceBuffer);

    // ---- 定数バッファ（距離フェード・UVストレッチ） ----
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage     = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(WeatherParamsCB);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Renderer::GetDevice()->CreateBuffer(&cbd, nullptr, &m_ParamsBuffer);

    // ---- シェーダーの読み込みと入力レイアウトの作成 ----
    // 2ストリーム構成（板ポリ＋インスタンス）なので Renderer の共通ヘルパーは使わず自前で作る
    {
        FILE* file = fopen("shader\\weatherVS.cso", "rb");
        assert(file);

        long fsize = _filelength(_fileno(file));
        unsigned char* buffer = new unsigned char[fsize];
        fread(buffer, fsize, 1, file);
        fclose(file);

        Renderer::GetDevice()->CreateVertexShader(buffer, fsize, nullptr, &m_VertexShader);

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            // スロット0: 共有板ポリ（1頂点ごとに変わる）
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },

            // スロット1: インスタンスデータ（1粒ごとに変わる）
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Position.xyz + Rotation
            { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Scale.xy + Pad
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Color
        };

        Renderer::GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout),
            buffer, fsize, &m_InputLayout);

        delete[] buffer;
    }

    Renderer::CreatePixelShader(&m_PixelShader, "shader\\weatherPS.cso");

    // ---- テクスチャ（柔らかい丸グラデーション。雨は縦に伸ばしてスジに見せる） ----
    TexMetadata  metadata;
    ScratchImage image;
    HRESULT hr = LoadFromWICFile(L"asset\\texture\\particle.png", WIC_FLAGS_NONE, &metadata, image);
    if (SUCCEEDED(hr))
    {
        CreateShaderResourceView(Renderer::GetDevice(),
            image.GetImages(), image.GetImageCount(), metadata, &m_Texture);
    }
    assert(m_Texture);
}

void WeatherRenderer::Uninit()
{
    if (m_QuadVertexBuffer) { m_QuadVertexBuffer->Release(); m_QuadVertexBuffer = nullptr; }
    if (m_InstanceBuffer)   { m_InstanceBuffer->Release();   m_InstanceBuffer   = nullptr; }
    if (m_ParamsBuffer)     { m_ParamsBuffer->Release();     m_ParamsBuffer     = nullptr; }
    if (m_InputLayout)      { m_InputLayout->Release();      m_InputLayout      = nullptr; }
    if (m_VertexShader)     { m_VertexShader->Release();     m_VertexShader     = nullptr; }
    if (m_PixelShader)      { m_PixelShader->Release();      m_PixelShader      = nullptr; }
    if (m_Texture)          { m_Texture->Release();          m_Texture          = nullptr; }
}

// ---------------------------------------------------------
// DrawBatch : 1バッチ（雨または雪）をDrawInstanced1回で描画する
// ---------------------------------------------------------
void WeatherRenderer::DrawBatch(const std::vector<WeatherInstance>& instances,
                                float fadeStart, float fadeEnd, float uvStretch)
{
    if (instances.empty()) return;

    int count = static_cast<int>(instances.size());
    if (count > m_MaxInstances) count = m_MaxInstances;

    auto* ctx = Renderer::GetDeviceContext();

    // ---- インスタンスデータをGPUへ転送（Map 1回） ----
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(m_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    memcpy(mapped.pData, instances.data(), sizeof(WeatherInstance) * count);
    ctx->Unmap(m_InstanceBuffer, 0);

    // ---- 距離フェード・UVストレッチの定数バッファを更新 ----
    WeatherParamsCB cb{};
    cb.FadeStart = fadeStart;
    cb.FadeEnd   = fadeEnd;
    cb.UVStretch = uvStretch;
    ctx->UpdateSubresource(m_ParamsBuffer, 0, nullptr, &cb, 0, 0);
    ctx->VSSetConstantBuffers(10, 1, &m_ParamsBuffer);
    ctx->PSSetConstantBuffers(10, 1, &m_ParamsBuffer);

    // ---- パイプライン設定 ----
    ctx->IASetInputLayout(m_InputLayout);
    ctx->VSSetShader(m_VertexShader, nullptr, 0);
    ctx->PSSetShader(m_PixelShader, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &m_Texture);

    ID3D11Buffer* buffers[2] = { m_QuadVertexBuffer, m_InstanceBuffer };
    UINT strides[2] = { sizeof(QuadVertex), sizeof(WeatherInstance) };
    UINT offsets[2] = { 0, 0 };
    ctx->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // AlphaBlend（Additiveは使わない）・深度書き込みなしで描画する
    // ※SetDepthEnable(false) は深度「書き込み」を止めるステート（テストは有効のまま）。
    //   壁の向こうの雨は正しく隠れつつ、半透明同士の順序問題を軽減できる
    Renderer::SetAdditiveBlend(false);
    Renderer::SetDepthEnable(false);

    ctx->DrawInstanced(4, count, 0, 0);
    m_DrawCalls++;

    Renderer::SetDepthEnable(true);
}
