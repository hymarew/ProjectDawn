// ===================================================
// particleSystemGPU.cpp
// フルGPUパーティクルの実行クラス
//
// 【フレームの流れ】
//   1. AddEmitRequest : CPUが「どこに何を何個」を積む（エミッタから）
//   2. Update         : Spawn CS（リクエストぶん）→ Update CS（走査範囲ぶん）
//                       生存粒子はブレンド別AppendBufferへ詰まれ、
//                       そのカウンタを間接引数バッファへコピーする
//   3. Draw           : DrawInstancedIndirect ×2（Step4で実装）
//
// 【走査範囲（ハイウォーターマーク）のCPUミラー】
//   GPU上のリングカーソルと同じ値をCPU側でも計算できる
//   （進む量 = 依頼した生成数の合計だから）。
//   さらに生成区間ごとに「最大寿命の期限」を記録しておき、
//   期限切れで走査範囲を縮める。全区間が期限切れになったら
//   カーソル自体を0へ巻き戻す（全粒子が死んでいるので安全）。
// ===================================================

#include "main.h"
#include "particleSystemGPU.h"
#include "renderer.h"
#include "DirectXTex.h"
#include <cstring>
#include <cstdio>
#include <io.h>

namespace
{
    // Texture2DArray に統合するテクスチャ（インデックス = 配列スライス番号）
    const wchar_t* TEXTURE_PATHS[] =
    {
        L"asset\\texture\\particle.png", // 0: デフォルト
        L"asset\\texture\\smoke.png",    // 1
        L"asset\\texture\\white.png",    // 2
    };
    constexpr int TEXTURE_COUNT = _countof(TEXTURE_PATHS);

    constexpr int THREAD_GROUP_SIZE = 256; // CS の numthreads と一致させること

    void LogFail(const char* what)
    {
        OutputDebugStringA("[ParticleSystemGPU] Init failed: ");
        OutputDebugStringA(what);
        OutputDebugStringA("\n");
    }
}

// ---------------------------------------------------------
// Init : 全GPUリソースの確保
// ---------------------------------------------------------
bool ParticleSystemGPU::Init(int poolSize)
{
    m_PoolSize = poolSize;

    m_Valid = CreateBuffers(poolSize) && CreateTextureArray() && CreateShaders();

    m_CursorSlot  = 0;
    m_ScanCount   = 0;
    m_WrappedOnce = false;
    m_AccumTime   = 0.0f;
    m_Expiry.clear();
    m_Requests.clear();

    return m_Valid;
}

// ---------------------------------------------------------
// CreateBuffers : 粒子プール・描画リスト・間接引数・定数バッファ
// ---------------------------------------------------------
bool ParticleSystemGPU::CreateBuffers(int poolSize)
{
    ID3D11Device* device = Renderer::GetDevice();
    HRESULT hr;

    // ---- 粒子プール（StructuredBuffer<GPUParticle>、96MB @100万粒子） ----
    // 全スロットを0初期化する（LifeTime=0 = 死亡スロットとして扱われる）
    {
        std::vector<uint8_t> zero((size_t)poolSize * sizeof(GPUParticle), 0);

        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth           = poolSize * sizeof(GPUParticle);
        bd.Usage               = D3D11_USAGE_DEFAULT;
        bd.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
        bd.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(GPUParticle);

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = zero.data();

        hr = device->CreateBuffer(&bd, &sd, &m_ParticlePool);
        if (FAILED(hr)) { LogFail("ParticlePool"); return false; }

        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.Format              = DXGI_FORMAT_UNKNOWN;
        ud.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
        ud.Buffer.NumElements  = poolSize;

        hr = device->CreateUnorderedAccessView(m_ParticlePool, &ud, &m_ParticlePoolUAV);
        if (FAILED(hr)) { LogFail("ParticlePoolUAV"); return false; }
    }

    // ---- リングカーソル（RWByteAddressBuffer。先頭4バイトのみ使用） ----
    {
        uint32_t zero[4] = {};

        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = sizeof(zero);
        bd.Usage     = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = zero;

        hr = device->CreateBuffer(&bd, &sd, &m_RingCursor);
        if (FAILED(hr)) { LogFail("RingCursor"); return false; }

        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.Format             = DXGI_FORMAT_R32_TYPELESS; // Rawビューは R32_TYPELESS 固定
        ud.ViewDimension      = D3D11_UAV_DIMENSION_BUFFER;
        ud.Buffer.NumElements = 4;
        ud.Buffer.Flags       = D3D11_BUFFER_UAV_FLAG_RAW;

        hr = device->CreateUnorderedAccessView(m_RingCursor, &ud, &m_RingCursorUAV);
        if (FAILED(hr)) { LogFail("RingCursorUAV"); return false; }
    }

    // ---- 描画リスト×2（AppendStructuredBuffer<ParticleDrawData>） ----
    for (int i = 0; i < 2; i++)
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth           = poolSize * sizeof(ParticleDrawData);
        bd.Usage               = D3D11_USAGE_DEFAULT;
        bd.BindFlags           = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        bd.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(ParticleDrawData);

        hr = device->CreateBuffer(&bd, nullptr, &m_DrawList[i]);
        if (FAILED(hr)) { LogFail("DrawList"); return false; }

        // Appendカウンタ付きUAV（Update CSが詰める側）
        D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
        ud.Format             = DXGI_FORMAT_UNKNOWN;
        ud.ViewDimension      = D3D11_UAV_DIMENSION_BUFFER;
        ud.Buffer.NumElements = poolSize;
        ud.Buffer.Flags       = D3D11_BUFFER_UAV_FLAG_APPEND;

        hr = device->CreateUnorderedAccessView(m_DrawList[i], &ud, &m_DrawListUAV[i]);
        if (FAILED(hr)) { LogFail("DrawListUAV"); return false; }

        // 描画VSが読む側
        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format              = DXGI_FORMAT_UNKNOWN;
        sd.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
        sd.Buffer.NumElements  = poolSize;

        hr = device->CreateShaderResourceView(m_DrawList[i], &sd, &m_DrawListSRV[i]);
        if (FAILED(hr)) { LogFail("DrawListSRV"); return false; }
    }

    // ---- 間接引数バッファ×2（DrawInstancedIndirect 用） ----
    // レイアウト: [VertexCountPerInstance, InstanceCount, StartVertex, StartInstance]
    // InstanceCount は毎フレーム CopyStructureCount で上書きされる
    for (int i = 0; i < 2; i++)
    {
        uint32_t args[4] = { 4, 0, 0, 0 }; // 板ポリ4頂点 × 0インスタンス

        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = sizeof(args);
        bd.Usage     = D3D11_USAGE_DEFAULT;
        bd.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = args;

        hr = device->CreateBuffer(&bd, &sd, &m_IndirectArgs[i]);
        if (FAILED(hr)) { LogFail("IndirectArgs"); return false; }
    }

    // ---- アクティブ数読み戻し用ステージング×2（ダブルバッファで遅延読み） ----
    // 各バッファに [通常合成の引数16B][加算合成の引数16B] の32バイトをコピーする
    for (int i = 0; i < 2; i++)
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth      = 32;
        bd.Usage          = D3D11_USAGE_STAGING;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        hr = device->CreateBuffer(&bd, nullptr, &m_CountStaging[i]);
        if (FAILED(hr)) { LogFail("CountStaging"); return false; }

        m_StagingValid[i] = false;
    }

    // ---- 定数バッファ（毎フレームMapするのでDYNAMIC） ----
    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        bd.ByteWidth = sizeof(EmitRequestParams);
        hr = device->CreateBuffer(&bd, nullptr, &m_EmitCB);
        if (FAILED(hr)) { LogFail("EmitCB"); return false; }

        bd.ByteWidth = sizeof(UpdateParamsGPU);
        hr = device->CreateBuffer(&bd, nullptr, &m_UpdateCB);
        if (FAILED(hr)) { LogFail("UpdateCB"); return false; }
    }

    return true;
}

// ---------------------------------------------------------
// CreateTextureArray : パーティクル用テクスチャ3種を1つの
// Texture2DArray へ統合する（描画を2ドローコールにするための下準備）
// ---------------------------------------------------------
bool ParticleSystemGPU::CreateTextureArray()
{
    using namespace DirectX;

    // ---- 全テクスチャをロードし、最大サイズを求める ----
    ScratchImage images[TEXTURE_COUNT];
    size_t maxW = 0, maxH = 0;

    for (int i = 0; i < TEXTURE_COUNT; i++)
    {
        TexMetadata meta;
        HRESULT hr = LoadFromWICFile(TEXTURE_PATHS[i], WIC_FLAGS_FORCE_RGB, &meta, images[i]);
        if (FAILED(hr)) { LogFail("LoadTexture"); return false; }

        if (meta.width  > maxW) maxW = meta.width;
        if (meta.height > maxH) maxH = meta.height;
    }

    // ---- サイズ・フォーマットを揃える（配列の全スライスは同一である必要がある） ----
    std::vector<ScratchImage> converted(TEXTURE_COUNT); // リサイズ/変換の結果保持用
    D3D11_SUBRESOURCE_DATA initData[TEXTURE_COUNT] = {};

    for (int i = 0; i < TEXTURE_COUNT; i++)
    {
        const Image* img = images[i].GetImage(0, 0, 0);

        // サイズ違いは最大サイズへ拡大（見た目はUVで正規化されるため影響なし）
        if (img->width != maxW || img->height != maxH)
        {
            HRESULT hr = Resize(*img, maxW, maxH, TEX_FILTER_LINEAR, converted[i]);
            if (FAILED(hr)) { LogFail("ResizeTexture"); return false; }
            img = converted[i].GetImage(0, 0, 0);
        }

        // フォーマットは R8G8B8A8_UNORM に統一する
        if (img->format != DXGI_FORMAT_R8G8B8A8_UNORM)
        {
            ScratchImage tmp;
            HRESULT hr = Convert(*img, DXGI_FORMAT_R8G8B8A8_UNORM,
                                 TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, tmp);
            if (FAILED(hr)) { LogFail("ConvertTexture"); return false; }
            converted[i] = std::move(tmp);
            img = converted[i].GetImage(0, 0, 0);
        }

        initData[i].pSysMem     = img->pixels;
        initData[i].SysMemPitch = (UINT)img->rowPitch;
    }

    // ---- Texture2DArray と SRV の作成 ----
    D3D11_TEXTURE2D_DESC td{};
    td.Width            = (UINT)maxW;
    td.Height           = (UINT)maxH;
    td.MipLevels        = 1;
    td.ArraySize        = TEXTURE_COUNT;
    td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = Renderer::GetDevice()->CreateTexture2D(&td, initData, &texture);
    if (FAILED(hr)) { LogFail("CreateTexture2DArray"); return false; }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format                         = td.Format;
    sd.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    sd.Texture2DArray.MipLevels       = 1;
    sd.Texture2DArray.ArraySize       = TEXTURE_COUNT;

    hr = Renderer::GetDevice()->CreateShaderResourceView(texture, &sd, &m_TextureArraySRV);
    texture->Release(); // SRVが参照を保持するため即座に手放してよい
    if (FAILED(hr)) { LogFail("TextureArraySRV"); return false; }

    return true;
}

// ---------------------------------------------------------
// CreateShaders : Spawn / Update のコンピュートシェーダー
// ---------------------------------------------------------
bool ParticleSystemGPU::CreateShaders()
{
    Renderer::CreateComputeShader(&m_SpawnCS,  "shader\\particleSpawnCS.cso");
    Renderer::CreateComputeShader(&m_UpdateCS, "shader\\particleUpdateCS.cso");

    if (!m_SpawnCS || !m_UpdateCS) { LogFail("ComputeShaders"); return false; }

    // 間接描画用のVS/PS。VSは頂点入力を持たないため入力レイアウトは作らない
    Renderer::CreatePixelShader(&m_DrawPS, "shader\\particleGPUPS.cso");

    {
        FILE* file = fopen("shader\\particleGPUVS.cso", "rb");
        if (!file) { LogFail("particleGPUVS.cso"); return false; }

        long fsize = _filelength(_fileno(file));
        std::vector<unsigned char> buffer(fsize);
        fread(buffer.data(), fsize, 1, file);
        fclose(file);

        Renderer::GetDevice()->CreateVertexShader(buffer.data(), fsize, nullptr, &m_DrawVS);
    }

    if (!m_DrawVS || !m_DrawPS) { LogFail("DrawShaders"); return false; }
    return true;
}

// ---------------------------------------------------------
// Uninit : 全GPUリソースの解放
// ---------------------------------------------------------
void ParticleSystemGPU::Uninit()
{
    auto SafeRelease = [](auto*& p) { if (p) { p->Release(); p = nullptr; } };

    SafeRelease(m_ParticlePool);
    SafeRelease(m_ParticlePoolUAV);
    SafeRelease(m_RingCursor);
    SafeRelease(m_RingCursorUAV);
    for (int i = 0; i < 2; i++)
    {
        SafeRelease(m_DrawList[i]);
        SafeRelease(m_DrawListUAV[i]);
        SafeRelease(m_DrawListSRV[i]);
        SafeRelease(m_IndirectArgs[i]);
        SafeRelease(m_CountStaging[i]);
    }
    SafeRelease(m_EmitCB);
    SafeRelease(m_UpdateCB);
    SafeRelease(m_SpawnCS);
    SafeRelease(m_UpdateCS);
    SafeRelease(m_DrawVS);
    SafeRelease(m_DrawPS);
    SafeRelease(m_TextureArraySRV);

    m_Valid = false;
}

// ---------------------------------------------------------
// TextureIndexFromPath : テクスチャパス → 配列スライス番号
// ---------------------------------------------------------
uint32_t ParticleSystemGPU::TextureIndexFromPath(const wchar_t* path) const
{
    if (!path) return 0;
    for (int i = 0; i < TEXTURE_COUNT; i++)
    {
        if (wcscmp(path, TEXTURE_PATHS[i]) == 0)
            return (uint32_t)i;
    }
    return 0; // 未知のパスはデフォルト（particle.png）
}

// ---------------------------------------------------------
// BuildParams : ParticleSetting → Spawn CS の定数バッファ値
// ---------------------------------------------------------
EmitRequestParams ParticleSystemGPU::BuildParams(
    const ParticleSetting& setting, const Vector3& position, int count)
{
    uint32_t flags = 0;
    if (setting.Additive)        flags |= PARTICLE_FLAG_ADDITIVE;
    if (setting.GroundAligned)   flags |= PARTICLE_FLAG_GROUND_ALIGNED;
    if (setting.GroundCollision) flags |= PARTICLE_FLAG_GROUND_COLLIDE;
    flags |= TextureIndexFromPath(setting.TexturePath) << PARTICLE_TEX_INDEX_SHIFT;

    EmitRequestParams params{};
    params.EmitPosition   = { position.x, position.y, position.z };
    params.PositionJitter = setting.PositionJitter;
    params.MinLife        = setting.MinLife;
    params.MaxLife        = setting.MaxLife;
    params.MinSpeed       = setting.MinSpeed;
    params.MaxSpeed       = setting.MaxSpeed;
    params.StartSize      = setting.StartSize;
    params.EndSize        = setting.EndSize;
    params.SizeVariance   = setting.SizeVariance;
    params.SpinSpeed      = setting.SpinSpeed;
    params.StartColor     = PackColor(setting.StartColor);
    params.EndColor       = PackColor(setting.EndColor);
    params.EmitCount      = (uint32_t)count;
    params.Seed           = m_SeedCounter++ * 2654435761u; // 呼び出しごとに異なるシード
    params.Drag           = setting.Drag;
    params.Turbulence     = setting.Turbulence;
    params.BuoyancyDelay  = setting.BuoyancyDelay;
    params.BuoyancyForce  = setting.BuoyancyForce;
    params.Bounciness     = setting.Bounciness;
    params.Flags          = flags;
    params.PoolSize       = (uint32_t)m_PoolSize;
    return params;
}

// ---------------------------------------------------------
// AddEmitRequest : 放出リクエストを積み、カーソルミラーを進める
// ---------------------------------------------------------
void ParticleSystemGPU::AddEmitRequest(
    const ParticleSetting& setting, const Vector3& position, int count)
{
    if (!m_Valid || count <= 0) return;
    if (count > m_PoolSize) count = m_PoolSize; // プール全長を超える依頼は打ち切る

    m_Requests.push_back(BuildParams(setting, position, count));
    m_SpawnedThisFrame += count;

    // ---- リングカーソルのCPUミラーを進める ----
    int end = m_CursorSlot + count;
    if (end >= m_PoolSize)
        m_WrappedOnce = true; // 一周した。以降は全域走査（全区間の期限切れでリセット）
    m_CursorSlot = end % m_PoolSize;

    // ---- 走査範囲と期限を記録する ----
    // この区間の粒子は「今 + 最大寿命」を過ぎれば確実に全滅している
    const int endSlot = m_WrappedOnce ? m_PoolSize : end;
    m_Expiry.push_back({ endSlot, m_AccumTime + setting.MaxLife });

    if (endSlot > m_ScanCount)
        m_ScanCount = endSlot;
}

// ---------------------------------------------------------
// Update : Spawn CS → Update CS を実行する
// ---------------------------------------------------------
void ParticleSystemGPU::Update(float dt, float groundY, float gravityY)
{
    if (!m_Valid) return;

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    m_AccumTime += dt;

    // ---- 期限切れ区間の削除と走査範囲の再計算 ----
    {
        int maxEnd = 0;
        size_t write = 0;
        for (size_t i = 0; i < m_Expiry.size(); i++)
        {
            if (m_Expiry[i].expiryTime <= m_AccumTime)
                continue; // 期限切れ: この区間の粒子は全滅済み
            if (m_Expiry[i].endSlot > maxEnd) maxEnd = m_Expiry[i].endSlot;
            m_Expiry[write++] = m_Expiry[i];
        }
        m_Expiry.resize(write);

        if (m_Expiry.empty() && m_Requests.empty())
        {
            // 全粒子が死んでいる → GPUリングカーソルを0へ巻き戻して
            // 走査範囲を完全リセットする（大量バースト後の回収）
            if (m_ScanCount > 0)
            {
                uint32_t zero[4] = {};
                context->UpdateSubresource(m_RingCursor, 0, nullptr, zero, 0, 0);
                m_CursorSlot  = 0;
                m_WrappedOnce = false;
            }
            m_ScanCount = 0;
        }
        else if (!m_WrappedOnce)
        {
            m_ScanCount = maxEnd; // 期限切れで末尾側が縮む
        }
        // WrappedOnce 中は m_ScanCount = m_PoolSize のまま（AddEmitRequest が設定済み）
    }

    // ---- Spawn CS : 積まれたリクエストを順に Dispatch ----
    if (!m_Requests.empty())
    {
        context->CSSetShader(m_SpawnCS, nullptr, 0);

        ID3D11UnorderedAccessView* uavs[2] = { m_ParticlePoolUAV, m_RingCursorUAV };
        UINT initialCounts[2] = { (UINT)-1, (UINT)-1 }; // カウンタなしUAVでは無視される
        context->CSSetUnorderedAccessViews(0, 2, uavs, initialCounts);

        for (const EmitRequestParams& request : m_Requests)
        {
            // 定数バッファへリクエストを書き込む
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(context->Map(m_EmitCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                memcpy(mapped.pData, &request, sizeof(request));
                context->Unmap(m_EmitCB, 0);
            }
            context->CSSetConstantBuffers(0, 1, &m_EmitCB);

            const UINT groups = (request.EmitCount + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
            context->Dispatch(groups, 1, 1);
        }
        m_Requests.clear();
    }

    // ---- Update CS : 走査範囲の全スロットを物理演算し描画リストを構築 ----
    if (m_ScanCount > 0)
    {
        UpdateParamsGPU params{};
        params.DeltaTime = dt;
        params.GroundY   = groundY;
        params.GravityY  = gravityY;
        params.ScanCount = (uint32_t)m_ScanCount;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(m_UpdateCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, &params, sizeof(params));
            context->Unmap(m_UpdateCB, 0);
        }
        context->CSSetConstantBuffers(0, 1, &m_UpdateCB);
        context->CSSetShader(m_UpdateCS, nullptr, 0);

        // Appendカウンタを0にリセットしつつバインドする（initialCounts=0）
        ID3D11UnorderedAccessView* uavs[3] =
            { m_ParticlePoolUAV, m_DrawListUAV[0], m_DrawListUAV[1] };
        UINT initialCounts[3] = { (UINT)-1, 0, 0 };
        context->CSSetUnorderedAccessViews(0, 3, uavs, initialCounts);

        const UINT groups = ((UINT)m_ScanCount + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
        context->Dispatch(groups, 1, 1);

        // ---- 描画数（Appendカウンタ）を間接引数バッファの InstanceCount 位置へコピー ----
        context->CopyStructureCount(m_IndirectArgs[0], sizeof(uint32_t), m_DrawListUAV[0]);
        context->CopyStructureCount(m_IndirectArgs[1], sizeof(uint32_t), m_DrawListUAV[1]);
    }

    // ---- UAVのバインドを解除する（描画パスがSRVとして読めるように） ----
    ID3D11UnorderedAccessView* nullUAVs[3] = {};
    context->CSSetUnorderedAccessViews(0, 3, nullUAVs, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);

    // ---- 統計: アクティブ数の遅延読み戻し（ダブルバッファ） ----
    // 今フレームのカウントを片方へコピーし、前回コピーした側を読む。
    // 直前のGPU処理を待たないのでストールしない（値は1〜2フレーム遅れ）
    {
        const int writeIdx = m_StagingCursor;
        const int readIdx  = 1 - m_StagingCursor;

        D3D11_BOX box{ 0, 0, 0, 16, 1, 1 };
        context->CopySubresourceRegion(m_CountStaging[writeIdx], 0,  0, 0, 0,
                                       m_IndirectArgs[0], 0, &box);
        context->CopySubresourceRegion(m_CountStaging[writeIdx], 0, 16, 0, 0,
                                       m_IndirectArgs[1], 0, &box);
        m_StagingValid[writeIdx] = true;

        if (m_StagingValid[readIdx])
        {
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(context->Map(m_CountStaging[readIdx], 0, D3D11_MAP_READ, 0, &mapped)))
            {
                const uint32_t* data = (const uint32_t*)mapped.pData;
                m_LastActiveCount = (int)(data[1] + data[5]); // 各引数の InstanceCount
                context->Unmap(m_CountStaging[readIdx], 0);
            }
        }
        m_StagingCursor = readIdx;
    }

    // 統計を確定して次フレームの集計に備える
    m_SpawnedLastFrame = m_SpawnedThisFrame;
    m_SpawnedThisFrame = 0;
}

// ---------------------------------------------------------
// Draw : 描画リストを DrawInstancedIndirect ×2 で描画する
//
// インスタンス数は間接引数バッファに GPU が書き込み済みのため、
// CPU は「何個描くか」を知らないまま描画を発行できる（読み戻しゼロ）。
// ---------------------------------------------------------
void ParticleSystemGPU::Draw()
{
    if (!m_Valid || m_ScanCount <= 0) return;

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // ---- 板ポリはVSが SV_VertexID から生成するため頂点バッファ不要 ----
    context->IASetInputLayout(nullptr);
    ID3D11Buffer* nullVB    = nullptr;
    UINT          zeroValue = 0;
    context->IASetVertexBuffers(0, 1, &nullVB, &zeroValue, &zeroValue);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(m_DrawVS, nullptr, 0);
    context->PSSetShader(m_DrawPS, nullptr, 0);
    context->PSSetShaderResources(0, 1, &m_TextureArraySRV);

    // 深度テスト無効（CPU版 ParticleRenderer と同じ。壁の後ろに隠れないように）
    Renderer::SetDepthEnable(false);

    // ---- pass 0: 通常合成 / pass 1: 加算合成（CPU版と同じ描画順） ----
    for (int pass = 0; pass < 2; pass++)
    {
        Renderer::SetAdditiveBlend(pass == 1);

        // VSが読む描画リストを切り替えて間接描画
        context->VSSetShaderResources(0, 1, &m_DrawListSRV[pass]);
        context->DrawInstancedIndirect(m_IndirectArgs[pass], 0);
    }

    // ---- ステートを元に戻す ----
    Renderer::SetAdditiveBlend(false);
    Renderer::SetDepthEnable(true);

    // 描画リストSRVのバインドを解除する
    // （次フレームの Update CS が同じバッファをUAVとしてバインドするため）
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->VSSetShaderResources(0, 1, &nullSRV);
}
