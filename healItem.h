#pragma once
#include "gameObject.h"
#include <d3d11.h>

// =====================================================
// HealItem : 回復アイテム
//
// プレイヤーが接触すると最大HPの10%を回復し（最大HPを超えない）、
// 緑のキラキラした回復エフェクトを再生して自身を削除する。
//
// 使い方:
//   auto* item = Manager::AddGameObject<HealItem>();
//   item->SetPosition({ 5.0f, 0.0f, 10.0f });
//
// 取得判定は SphereCollider の半径（GameConfig::HealItem::PICKUP_RADIUS）
// とプレイヤー半径の距離チェックで行う。
// =====================================================
class HealItem : public GameObject
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
    void DrawShadow()     override;
    const char* GetName() override { return "HealItem"; }

    static void ReleaseShaders();

private:
    // 多重取得防止フラグ。取得した瞬間 true になり、以後の接触判定をスキップする
    bool m_Consumed = false;

    LIGHT Light = {};

    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static ID3D11InputLayout*  m_VertexLayout;
    static bool                m_IsLoaded;
};
