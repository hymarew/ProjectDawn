#pragma once
#include "vector3.h"
#include <d3d11.h>

constexpr int MAX_DYNAMIC_LIGHTS = 4;

// =====================================================
// DynamicLightManager : ロケットの噴射炎・爆発フラッシュ等、
// 短時間だけ周囲を照らす小さなポイントライトを管理する。
//
// 固定4スロットのみで、new/deleteは一切行わない（オブジェクトプール的な扱い）。
//
// 使い方:
//   追従が必要なライト（ロケット本体の噴射炎等）:
//     int slot = g_DynamicLightManager.Acquire(pos, radius, color, intensity);
//     毎フレーム: g_DynamicLightManager.UpdateSlot(slot, newPos);
//     不要になったら: g_DynamicLightManager.Release(slot);
//
//   一瞬だけの閃光（爆発フラッシュ等）:
//     g_DynamicLightManager.AddFlash(pos, radius, color, intensity, life);
//     （寿命が尽きると自動的にスロットが解放される）
//
//   毎フレーム: Update(dt) で寿命付きライトの時間を進め、
//              Apply() でGPUの定数バッファ(b10)へ送る（フレームに1回でよい）。
// =====================================================
class DynamicLightManager
{
public:
    void Init();
    void Uninit();

    void Update(float dt); // 寿命付きライト（AddFlashで追加したもの）の時間管理
    void Apply();          // 現在の状態をGPUへ送る。フレーム中の描画開始前に1回呼べばよい

    // 追従が必要な常時点灯ライトを確保する。戻り値 -1 は空きスロットなし
    int  Acquire(const Vector3& pos, float radius, const Vector3& color, float intensity);
    void UpdateSlot(int slot, const Vector3& pos);
    void Release(int slot);

    // 寿命付きの一時的な閃光を追加する（既存の空きスロットを使い、寿命が尽きたら自動解放）
    void AddFlash(const Vector3& pos, float radius, const Vector3& color, float intensity, float life);

private:
    struct Slot
    {
        bool    active    = false;
        Vector3 position  = {};
        float   radius    = 0.0f;
        Vector3 color     = { 1.0f, 1.0f, 1.0f };
        float   intensity = 0.0f;

        bool    isFlash    = false; // true: Update() で寿命管理して自動的にRelease相当になる
        float   life       = 0.0f;  // 残り時間（isFlashのみ使用）
        float   maxLife    = 0.0f;  // 減衰計算用
    };

    Slot          m_Slots[MAX_DYNAMIC_LIGHTS];
    ID3D11Buffer* m_Buffer = nullptr;

    int FindFreeSlot() const;
};

extern DynamicLightManager g_DynamicLightManager;
