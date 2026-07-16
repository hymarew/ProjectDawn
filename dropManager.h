#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "vector3.h"

class DropTableDatabase;
class ItemFactory;
class IDropStrategy;

// =====================================================
// DropManager : 敵撃破時のドロップ処理の入口
//
//   Enemy::Kill()
//     → OnEnemyKilled(TypeName, 座標)
//     → DropTableDatabase からテーブル検索
//     → strategyType に対応する IDropStrategy で抽選
//     → ItemFactory でワールドアイテムを生成（少し散らして配置）
//
// Enemy 側は TypeName と座標を渡すだけで、
// ドロップテーブル・抽選方法・アイテム生成の詳細を知らない。
// =====================================================
class DropManager
{
public:
    // 各依存はシーン初期化時に注入する（GameScene::Init から呼ぶ）
    void Init(const DropTableDatabase* dropDB, const ItemFactory* itemFactory);
    void Uninit();

    // 敵死亡時に Enemy::Kill() から呼ばれる
    void OnEnemyKilled(const char* enemyTypeName, const Vector3& position);

private:
    const DropTableDatabase* m_DropDB      = nullptr;
    const ItemFactory*       m_ItemFactory = nullptr;

    // strategyType 文字列 → 抽選 Strategy の登録表。
    // 新しい抽選方法を追加するときは Init に1行足す。
    std::unordered_map<std::string, std::unique_ptr<IDropStrategy>> m_Strategies;
};

extern DropManager g_DropManager;
