#pragma once
#include <d3d11.h>
#include <vector>
#include <cstdint>
#include "particleGPU.h"
#include "particleSetting.h"
#include "vector3.h"

// =====================================================
// ParticleSystemGPU : フルGPUパーティクルの実行クラス
//
// 【役割】
//   - 粒子プール・描画リスト等のGPUリソースを保持する
//   - CPUから受け取った EmitRequest を Spawn CS で粒子化する
//   - Update CS で物理演算し、描画リスト（AppendBuffer）を構築する
//   - DrawInstancedIndirect で描画する（描画数はGPUが決定）
//
// 【CPUの仕事】
//   「今フレームどこに何を何個出すか」（AddEmitRequest）と
//   走査範囲（ハイウォーターマーク）の管理だけ。
//   粒子データそのものはVRAM常駐でCPUへは戻さない。
//
// 所有者は ParticleManager。CPU版（既存経路）と切替式で共存する。
// =====================================================
class ParticleSystemGPU
{
public:
    // poolSize: リングバッファの全長（= GameConfig::Particle::POOL_SIZE）
    // 戻り値: 全リソースの確保に成功したら true（失敗時はGPU経路を無効化する）
    bool Init(int poolSize);
    void Uninit();

    // エフェクト1回分の放出リクエストを積む（ParticleEmitter から呼ばれる）
    // count: 生成数（BurstCount またはこのフレームの連続放出数）
    void AddEmitRequest(const ParticleSetting& setting, const Vector3& position, int count);

    // 積まれたリクエストの Spawn Dispatch → 全粒子の Update Dispatch を実行する
    void Update(float dt, float groundY, float gravityY);

    // 描画リストを DrawInstancedIndirect ×2（通常合成→加算合成）で描画する
    void Draw();

    // 直近フレームの統計（デバッグUI用。アクティブ数は2フレーム遅れ）
    int  GetActiveCount() const { return m_LastActiveCount; }
    int  GetSpawnedLastFrame() const { return m_SpawnedLastFrame; }
    int  GetScanCount() const { return m_ScanCount; }

    bool IsValid() const { return m_Valid; }

private:
    // ---- 内部ヘルパー ----
    bool CreateBuffers(int poolSize);
    bool CreateTextureArray();
    bool CreateShaders();

    // テクスチャパス → Texture2DArray のインデックス（未知のパスは0）
    uint32_t TextureIndexFromPath(const wchar_t* path) const;

    // ParticleSetting から EmitRequestParams（CS定数バッファ）を組み立てる
    EmitRequestParams BuildParams(const ParticleSetting& setting,
                                  const Vector3& position, int count);

    // ---- 走査範囲（ハイウォーターマーク）のCPU側ミラー ----
    // リングカーソルはGPU上にあるが、CPUも「何個生成を依頼したか」を知っているので
    // 読み戻しなしで同じカーソル位置を再現できる。
    // 走査範囲の縮小は「区間ごとの最大寿命の期限切れ」で行う（ExpiryRecord）。
    struct ExpiryRecord
    {
        int   endSlot;     // この生成区間の終了カーソル位置（mod前の通し番号ではなくスロット換算）
        float expiryTime;  // この区間の全粒子が確実に死んでいる時刻（accumTime基準）
    };

    int   m_PoolSize   = 0;
    int   m_CursorSlot = 0;     // GPUリングカーソルのCPUミラー（mod PoolSize済み）
    int   m_ScanCount  = 0;     // Update CS に渡す走査スロット数
    bool  m_WrappedOnce = false; // カーソルが一周したら常に全域走査
    float m_AccumTime  = 0.0f;  // 起動からの累計時間（期限判定用）
    std::vector<ExpiryRecord> m_Expiry;

    // ---- 今フレームの EmitRequest ----
    std::vector<EmitRequestParams> m_Requests;
    uint32_t m_SeedCounter      = 1;
    int      m_SpawnedThisFrame = 0;  // 集計中（AddEmitRequest が加算）
    int      m_SpawnedLastFrame = 0;  // 確定値（デバッグUI表示用）

    // ---- GPUリソース ----
    ID3D11Buffer*              m_ParticlePool      = nullptr; // StructuredBuffer<GPUParticle>
    ID3D11UnorderedAccessView* m_ParticlePoolUAV   = nullptr;

    ID3D11Buffer*              m_RingCursor        = nullptr; // RWByteAddressBuffer（4バイト）
    ID3D11UnorderedAccessView* m_RingCursorUAV     = nullptr;

    // 描画リスト（[0]=通常合成 / [1]=加算合成）
    ID3D11Buffer*              m_DrawList[2]       = {};      // AppendStructuredBuffer<ParticleDrawData>
    ID3D11UnorderedAccessView* m_DrawListUAV[2]    = {};
    ID3D11ShaderResourceView*  m_DrawListSRV[2]    = {};      // 描画VSが読む

    ID3D11Buffer*              m_IndirectArgs[2]   = {};      // DrawInstancedIndirect の引数
    ID3D11Buffer*              m_CountStaging[2]   = {};      // アクティブ数読み戻し用（遅延）

    ID3D11Buffer*              m_EmitCB            = nullptr; // EmitRequestParams
    ID3D11Buffer*              m_UpdateCB          = nullptr; // UpdateParamsGPU

    ID3D11ComputeShader*       m_SpawnCS           = nullptr;
    ID3D11ComputeShader*       m_UpdateCS          = nullptr;

    // 間接描画用シェーダー（頂点は SV_VertexID / SV_InstanceID から生成するため
    // 入力レイアウト・頂点バッファは持たない）
    ID3D11VertexShader*        m_DrawVS            = nullptr;
    ID3D11PixelShader*         m_DrawPS            = nullptr;

    // パーティクル用テクスチャをまとめた Texture2DArray
    ID3D11ShaderResourceView*  m_TextureArraySRV   = nullptr;

    // 統計
    int  m_LastActiveCount = 0;
    int  m_StagingCursor   = 0;    // 読み戻しのダブルバッファ切替
    bool m_StagingValid[2] = {};   // 各stagingにコピー済みデータがあるか

    bool m_Valid = false;
};
