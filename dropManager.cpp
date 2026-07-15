#include "dropManager.h"
#include "dropTableDatabase.h"
#include "itemFactory.h"
#include "weightedDropStrategy.h"
#include "bossDropStrategy.h"
#include "debugPrintf.h"
#include "GameConfig.h"
#include <cstdlib>

DropManager g_DropManager;

void DropManager::Init(const DropTableDatabase* dropDB, const ItemFactory* itemFactory)
{
    m_DropDB      = dropDB;
    m_ItemFactory = itemFactory;

    // strategyType → Strategy の登録表。
    // 新しい抽選方法（例: "Elite"）を追加するときはここに1行足す。
    m_Strategies.clear();
    m_Strategies["Weighted"] = std::make_unique<WeightedDropStrategy>();
    m_Strategies["Boss"]     = std::make_unique<BossDropStrategy>();
}

void DropManager::Uninit()
{
    m_DropDB      = nullptr;
    m_ItemFactory = nullptr;
    m_Strategies.clear();
}

// ---------------------------------------------------------
// OnEnemyKilled : テーブル検索 → 抽選 → アイテム生成
// ---------------------------------------------------------
void DropManager::OnEnemyKilled(const char* enemyTypeName, const Vector3& position)
{
    if (!m_DropDB || !m_ItemFactory) return;

    // テーブルがない敵は「何もドロップしない」扱い（エラーではない）
    const DropTableData* table = m_DropDB->Find(enemyTypeName);
    if (!table) return;

    auto it = m_Strategies.find(table->strategyType);
    if (it == m_Strategies.end())
    {
        DebugPrintf("[DropManager] unknown strategy '%s' (enemyType '%s')\n",
                    table->strategyType.c_str(), enemyTypeName);
        return;
    }

    std::vector<ItemID> drops;
    it->second->Select(*table, drops);

    // 複数ドロップ時に重ならないよう、倒れた場所の周囲へ少し散らして配置する
    for (const ItemID& id : drops)
    {
        const float scatter = GameConfig::WorldItem::DROP_SCATTER_RADIUS;
        Vector3 dropPos = position;
        dropPos.x += ((float)rand() / RAND_MAX * 2.0f - 1.0f) * scatter;
        dropPos.z += ((float)rand() / RAND_MAX * 2.0f - 1.0f) * scatter;
        dropPos.y  = 0.0f;  // 地面に配置（浮遊高さは WorldItem 側で加算する）

        m_ItemFactory->Spawn(id, dropPos);
    }
}
