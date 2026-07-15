#pragma once
#include <map>
#include <vector>
#include "weaponData.h"

class WeaponDatabase;

// =====================================================
// Inventory : プレイヤーの所持品管理
//
// 現状は武器のみを管理する（回復は取得と同時に使用するため所持しない）。
// 同一 WeaponID は重複所持しないため、WeaponID がそのまま所持武器の
// 一意キーになる。強化値など個体側の状態は OwnedWeapon に持たせる。
//
// 所持状態は SaveManager（save.json の unlocks セクション、キー "weapon101"）
// と連携して恒久化する。
//
// 将来: 素材・強化素材を追加するときはスタック管理のコンテナ
// （map<ItemID, int>）をここに追加する。
// =====================================================
class Inventory
{
public:
    // 所持武器1本分の個体データ（マスタ値は WeaponDatabase 側にある）
    struct OwnedWeapon
    {
        WeaponID weaponID;
        int      enhanceLevel = 0;     // 将来の武器強化用
        bool     isNew        = true;  // WeaponSelect 画面の "NEW" 表示用
    };

    enum class AddResult
    {
        NewlyAcquired,  // 新規獲得
        AlreadyOwned,   // 所持済み（所持数は増えない）
    };

    // SaveManager から所持状態を復元する。
    // セーブに存在しない WeaponID（データ削除等）は weaponDB と照合して除外する。
    // 1本も所持していない初回起動時は初期武器（AR / RL）を付与して保存する。
    void Init(const WeaponDatabase* weaponDB);

    // 武器を追加し、SaveManager 経由で即時保存する（所持済みなら何もしない）
    AddResult AddWeapon(WeaponID id);

    bool Has(WeaponID id) const;

    // WeaponSelect 画面で確認した武器の "NEW" 表示を消す
    void ClearNewFlag(WeaponID id);

    // ID 昇順の所持武器一覧（WeaponSelect 画面用）
    std::vector<const OwnedWeapon*> GetOwnedWeapons() const;

    int GetWeaponCount() const { return (int)m_Weapons.size(); }

private:
    // キーは WeaponID::number。map なので走査が常に ID 昇順になる
    std::map<int, OwnedWeapon> m_Weapons;

    const WeaponDatabase* m_WeaponDB = nullptr;
};
