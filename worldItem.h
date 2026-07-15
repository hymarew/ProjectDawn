#pragma once
#include "gameObject.h"
#include "renderer.h"
#include "itemData.h"
#include <d3d11.h>

// =====================================================
// WorldItem : フィールドに落ちているアイテム（プール管理）
//
// WorldItemPool が事前確保し、ItemFactory::Spawn() の Activate で起動する。
// Manager の GameObject リストには入れない（EnemyPool と同方式）。
//
// 責務は「見た目」のみ:
//   - 浮遊アニメーション + 回転
//   - レアリティ色の発光（ライト色で表現）
// 時間経過では消えない。消えるのは取得時（Deactivate）と
// シーン終了時のプールリセット（死亡・クリア・リタイア）のみ。
// 取得判定と取得時の効果適用は ItemPickupSystem が行う。
// =====================================================
class WorldItem : public GameObject
{
public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
    void DrawShadow()     override;
    const char* GetName() override { return "WorldItem"; }

    // ItemFactory から呼ばれる。ItemData の内容で表示を設定して起動する
    void Activate(const ItemData& data, const Vector3& pos);

    // 取得・寿命切れ時に呼ばれる。非アクティブへ戻す（プールへ返却）
    void Deactivate() { m_IsActive = false; }

    ItemID   GetItemID()  const { return m_ItemID; }
    ItemType GetItemType() const { return m_ItemType; }

    static void ReleaseShaders();

private:
    ItemID   m_ItemID;
    ItemType m_ItemType = ItemType::Heal;
    Rarity   m_Rarity   = Rarity::Common;

    float m_BobTimer = 0.0f;  // 浮遊アニメーション用の経過時間
    float m_BaseY    = 0.0f;  // 浮遊の基準高さ

    LIGHT Light = {};

    static ID3D11VertexShader* m_VertexShader;
    static ID3D11PixelShader*  m_PixelShader;
    static ID3D11InputLayout*  m_VertexLayout;
    static bool                m_IsLoaded;
};
