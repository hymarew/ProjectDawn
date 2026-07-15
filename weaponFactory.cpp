#include "weaponFactory.h"
#include "weapon.h"
#include "weaponDatabase.h"
#include "straightShotStrategy.h"
#include "splashProjectileStrategy.h"
#include "debugPrintf.h"

// ---------------------------------------------------------
// コンストラクタ : attackType → Strategy の登録表を構築する
//
// 新しい攻撃方式（例: "SpreadShot"）を追加するときはここに1行足す。
// ---------------------------------------------------------
WeaponFactory::WeaponFactory(const WeaponDatabase* database)
    : m_Database(database)
{
    m_StrategyRegistry["StraightShot"] = []() -> std::unique_ptr<IAttackStrategy>
        { return std::make_unique<StraightShotStrategy>(); };

    m_StrategyRegistry["SplashProjectile"] = []() -> std::unique_ptr<IAttackStrategy>
        { return std::make_unique<SplashProjectileStrategy>(); };
}

// ---------------------------------------------------------
// Create : WeaponID から Weapon を組み立てる
// ---------------------------------------------------------
std::unique_ptr<Weapon> WeaponFactory::Create(WeaponID id, BulletPool* pool) const
{
    if (!m_Database) return nullptr;

    const WeaponData* data = m_Database->Find(id);
    if (!data)
    {
        DebugPrintf("[WeaponFactory] unknown WeaponID %d\n", id.number);
        return nullptr;
    }

    auto it = m_StrategyRegistry.find(data->attackType);
    if (it == m_StrategyRegistry.end())
    {
        DebugPrintf("[WeaponFactory] unknown attackType '%s' (WeaponID %d)\n",
                    data->attackType.c_str(), id.number);
        return nullptr;
    }

    return std::make_unique<Weapon>(*data, it->second(), pool);
}

bool WeaponFactory::IsKnownAttackType(const std::string& attackType) const
{
    return m_StrategyRegistry.count(attackType) > 0;
}

// ---------------------------------------------------------
// ValidateDatabase : 全武器の attackType がすべて登録済みか検証する
// ---------------------------------------------------------
bool WeaponFactory::ValidateDatabase() const
{
    if (!m_Database) return false;

    bool ok = true;
    for (const WeaponData* data : m_Database->GetAll())
    {
        if (!IsKnownAttackType(data->attackType))
        {
            DebugPrintf("[WeaponFactory] weapon %d '%s' has unknown attackType '%s'\n",
                        data->id.number, data->name.c_str(), data->attackType.c_str());
            ok = false;
        }
    }
    return ok;
}
