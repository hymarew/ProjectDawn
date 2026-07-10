// ===================================================
// particleRenderer.cpp
// パーティクルのGPU描画をすべて担当するクラス
//
// 【GPUインスタンシングによる描画】
//   旧実装: パーティクル1個ごとに
//             テクスチャセット + 定数バッファ更新×2 + Draw(4,0)
//           → 1万個で1万ドローコール（CPUのAPI呼び出しが律速）
//
//   新実装: 全パーティクルのインスタンスデータを毎フレーム1回のMapで転送し、
//           (テクスチャ × ブレンド種別) のバケットごとに DrawInstanced を1回発行
//           → 10万個でもドローコールは最大6回程度
//
// 【描画フロー】
//   1. プールを走査し、アクティブな粒子をバケット（テクスチャ×ブレンド）へ振り分け
//   2. インスタンスバッファを Map(WRITE_DISCARD) し、
//      「通常合成 → 加算合成」の順にバケットを詰めて書き込む
//   3. バケットごとにテクスチャ/ブレンドをセットして DrawInstanced
//
// 【ビルボードとは】
//   板ポリゴンが常にカメラの方を向く技術。
//   行列の組み立ては particleVS.hlsl がGPU側で行う。
// ===================================================

#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "particleRenderer.h"
#include "DirectXTex.h"
#include <cassert>
#include <cstdio>
#include <io.h>

namespace
{
    // 共有板ポリの頂点（位置とUVだけの最小構成）
    struct QuadVertex
    {
        XMFLOAT3 Position;
        XMFLOAT2 TexCoord;
    };
}

// ---------------------------------------------------------
// Init : GPUリソースの確保とデフォルトテクスチャの読み込み
// ---------------------------------------------------------
void ParticleRenderer::Init(int maxInstances)
{
    m_InstanceCapacity = maxInstances;

    // ---- 板ポリゴンの頂点データを作る ----
    // 中心を原点とした1×1の四角形。全パーティクルがこの1枚を共有し、
    // インスタンスデータ（スロット1）でサイズ・位置・回転を変える。
    QuadVertex vertex[4];
    vertex[0] = { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT2(0, 0) }; // 左上
    vertex[1] = { XMFLOAT3( 0.5f,  0.5f, 0.0f), XMFLOAT2(1, 0) }; // 右上
    vertex[2] = { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0, 1) }; // 左下
    vertex[3] = { XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT2(1, 1) }; // 右下

    D3D11_BUFFER_DESC bd{};
    bd.Usage     = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(QuadVertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;

    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_QuadVertexBuffer);

    // ---- インスタンスバッファ（毎フレームCPUから書き込む） ----
    // DYNAMIC + WRITE_DISCARD で、フレームに1回だけ Map してまとめて転送する
    D3D11_BUFFER_DESC ibd{};
    ibd.Usage          = D3D11_USAGE_DYNAMIC;
    ibd.ByteWidth      = sizeof(ParticleInstance) * maxInstances;
    ibd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Renderer::GetDevice()->CreateBuffer(&ibd, nullptr, &m_InstanceBuffer);

    // ---- シェーダーの読み込みと入力レイアウトの作成 ----
    // スロット0（共有板ポリ）とスロット1（インスタンスデータ）の
    // 2ストリーム構成なので、Renderer の共通ヘルパーは使わず自前で作る
    {
        FILE* file = fopen("shader\\particleVS.cso", "rb");
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

            // スロット1: インスタンスデータ（1パーティクルごとに変わる）
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Position.xyz + Size
            { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Color
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Rotation + GroundAligned
        };

        Renderer::GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout),
            buffer, fsize, &m_InputLayout);

        delete[] buffer;
    }

    Renderer::CreatePixelShader(&m_PixelShader, "shader\\particlePS.cso");

    // ---- デフォルトテクスチャを読み込んでキャッシュに登録 ----
    // 他のテクスチャの読み込みに失敗したときのフォールバックとして使う
    m_DefaultTexture = GetOrLoadTexture(L"asset\\texture\\particle.png");
}

// ---------------------------------------------------------
// Uninit : GPUリソースの解放
// ---------------------------------------------------------
void ParticleRenderer::Uninit()
{
    if (m_QuadVertexBuffer) { m_QuadVertexBuffer->Release(); m_QuadVertexBuffer = nullptr; }
    if (m_InstanceBuffer)   { m_InstanceBuffer->Release();   m_InstanceBuffer   = nullptr; }
    if (m_InputLayout)      { m_InputLayout->Release();      m_InputLayout      = nullptr; }
    if (m_VertexShader)     { m_VertexShader->Release();     m_VertexShader     = nullptr; }
    if (m_PixelShader)      { m_PixelShader->Release();      m_PixelShader      = nullptr; }

    // テクスチャキャッシュの全エントリを解放
    for (auto& entry : m_TextureCache)
    {
        if (entry.second) entry.second->Release();
    }
    m_TextureCache.clear();
    m_DefaultTexture = nullptr;

    m_Buckets.clear();
}

// ---------------------------------------------------------
// GetOrLoadTexture : テクスチャをキャッシュ付きで取得
// ---------------------------------------------------------
ID3D11ShaderResourceView* ParticleRenderer::GetOrLoadTexture(const wchar_t* path)
{
    std::wstring key(path);

    // ---- キャッシュを検索 ----
    // 同じパスのテクスチャがすでに読み込まれていたらそれを返す（二重ロード防止）
    auto it = m_TextureCache.find(key);
    if (it != m_TextureCache.end())
        return it->second;

    // ---- テクスチャをファイルから読み込む ----
    TexMetadata  metadata;
    ScratchImage image;
    HRESULT hr = LoadFromWICFile(path, WIC_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
    {
        // 読み込み失敗時はデフォルトテクスチャで代替
        // （クラッシュせずに処理を続けられる）
        return m_DefaultTexture;
    }

    // ---- GPU がシェーダーで読めるビュー（SRV）を作成してキャッシュに登録 ----
    ID3D11ShaderResourceView* srv = nullptr;
    CreateShaderResourceView(Renderer::GetDevice(),
        image.GetImages(), image.GetImageCount(), metadata, &srv);

    m_TextureCache[key] = srv; // 次回以降はファイルを読まずキャッシュから返す
    return srv;
}

// ---------------------------------------------------------
// FindOrAddBucket : (テクスチャ, ブレンド) が一致するバケットを返す
// バケット数はテクスチャ種×2程度しかないため線形探索で十分速い
// ---------------------------------------------------------
ParticleRenderer::Bucket& ParticleRenderer::FindOrAddBucket(
    ID3D11ShaderResourceView* texture, bool additive)
{
    for (auto& bucket : m_Buckets)
    {
        if (bucket.Texture == texture && bucket.Additive == additive)
            return bucket;
    }

    m_Buckets.push_back({ texture, additive, {} });
    return m_Buckets.back();
}

// ---------------------------------------------------------
// Draw : アクティブな全パーティクルをインスタンシングで一括描画
// ---------------------------------------------------------
void ParticleRenderer::Draw(const ParticleData* pool, int poolSize)
{
    m_ActiveCount = 0;
    m_DrawCalls   = 0;

    // ---- 1. バケットへの振り分け ----
    // バケット自体（テクスチャ・ブレンドの組）は残したまま中身だけ空にする。
    // vector の capacity が維持されるので毎フレームのメモリ再確保が起きない。
    for (auto& bucket : m_Buckets)
        bucket.Instances.clear();

    // 同じエミッタ由来のパーティクルはプール上で連続していることが多いため、
    // 直前に解決したテクスチャをキャッシュして map 検索の回数を減らす
    const wchar_t*            lastPath = nullptr;
    ID3D11ShaderResourceView* lastSrv  = nullptr;

    for (int i = 0; i < poolSize; i++)
    {
        const ParticleData& p = pool[i];
        if (!p.Active) continue; // 非アクティブはスキップ

        ID3D11ShaderResourceView* srv;
        if (p.TexturePath == lastPath && lastSrv != nullptr)
        {
            srv = lastSrv;
        }
        else
        {
            srv = p.TexturePath ? GetOrLoadTexture(p.TexturePath) : m_DefaultTexture;
            lastPath = p.TexturePath;
            lastSrv  = srv;
        }

        ParticleInstance inst;
        inst.Position      = { p.Position.x, p.Position.y, p.Position.z };
        inst.Size          = p.Size;
        inst.Color         = p.Color;
        inst.Rotation      = p.Rotation;
        inst.GroundAligned = p.GroundAligned ? 1.0f : 0.0f;
        inst.Pad           = { 0.0f, 0.0f };

        FindOrAddBucket(srv, p.Additive).Instances.push_back(inst);
        m_ActiveCount++;
    }

    if (m_ActiveCount == 0)
        return; // 描くものがなければGPUステートに触らない

    // ---- 2. インスタンスバッファへ一括転送 ----
    // 描画順（通常合成 → 加算合成）と同じ並びで書き込み、
    // 各バケットの開始位置（StartInstanceLocation）を記録しておく
    D3D11_MAPPED_SUBRESOURCE mapped{};
    Renderer::GetDeviceContext()->Map(m_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    ParticleInstance* dst    = static_cast<ParticleInstance*>(mapped.pData);
    int               cursor = 0;

    // pass 0: 通常合成のバケット / pass 1: 加算合成のバケット
    std::vector<std::pair<Bucket*, int>> drawList; // (バケット, 開始インスタンス位置)
    for (int pass = 0; pass < 2; pass++)
    {
        const bool additive = (pass == 1);
        for (auto& bucket : m_Buckets)
        {
            if (bucket.Additive != additive || bucket.Instances.empty())
                continue;

            const int count = static_cast<int>(bucket.Instances.size());
            memcpy(dst + cursor, bucket.Instances.data(), sizeof(ParticleInstance) * count);
            drawList.push_back({ &bucket, cursor });
            cursor += count;
        }
    }

    Renderer::GetDeviceContext()->Unmap(m_InstanceBuffer, 0);

    // ---- 3. バケットごとに DrawInstanced ----
    Renderer::GetDeviceContext()->IASetInputLayout(m_InputLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

    ID3D11Buffer* vbs[2]     = { m_QuadVertexBuffer, m_InstanceBuffer };
    UINT          strides[2] = { sizeof(QuadVertex), sizeof(ParticleInstance) };
    UINT          offsets[2] = { 0, 0 };
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 2, vbs, strides, offsets);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 深度テストを無効化（パーティクルが壁の後ろに隠れないようにする）
    Renderer::SetDepthEnable(false);

    // ブレンドモードの現在値を追跡し、切り替わったときだけステートを変更する
    bool additiveEnabled = false;
    Renderer::SetAdditiveBlend(false);

    for (auto& entry : drawList)
    {
        Bucket* bucket        = entry.first;
        int     startInstance = entry.second;

        if (bucket->Additive != additiveEnabled)
        {
            additiveEnabled = bucket->Additive;
            Renderer::SetAdditiveBlend(additiveEnabled);
        }

        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &bucket->Texture);

        // 板ポリ4頂点 × バケット内の全インスタンスを1回のドローコールで描画
        Renderer::GetDeviceContext()->DrawInstanced(
            4,
            static_cast<UINT>(bucket->Instances.size()),
            0,
            static_cast<UINT>(startInstance));
        m_DrawCalls++;
    }

    // ブレンド・深度を元に戻す（他のオブジェクトの描画に影響しないよう）
    Renderer::SetAdditiveBlend(false);
    Renderer::SetDepthEnable(true);
}
