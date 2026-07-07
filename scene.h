#pragma once

// =====================================================
// Scene : 全シーンの基底クラス
//
// 各シーンはこのクラスを継承し、
// Init / Uninit / Update / Draw を実装する。
// =====================================================
class Scene
{
public:
    virtual ~Scene() = default;

    // シーン開始時に1回呼ばれる（オブジェクト生成など）
    virtual void Init() = 0;

    // シーン終了時に1回呼ばれる（メモリ解放など）
    virtual void Uninit() = 0;

    // 毎フレーム呼ばれる（ゲームロジック）
    virtual void Update(float dt) = 0;

    // 毎フレーム呼ばれる（描画）
    virtual void Draw() = 0;
};
