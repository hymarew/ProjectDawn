#pragma once
#include <vector>
#include <unordered_map>
#include "itemData.h"

class WeaponDatabase;

// =====================================================
// ItemDatabase : アイテムマスタデータの読み込みと検索
//
// Data/Items/*.json を走査して構築する。
// 武器アイテムは weaponID で WeaponDatabase を参照するため、
// Load 後に Validate() で参照切れを検出できる。
// =====================================================
class ItemDatabase
{
public:
    // Data/Items/*.json を読み込む。壊れたファイルはログを出してスキップする。
    bool Load();

    // 武器アイテムの weaponID が WeaponDatabase に存在するか検証する。
    // 参照切れはログへ警告を出す（該当アイテムは残すが、取得時は無視される）。
    // 戻り値: 参照切れが1件もなければ true
    bool Validate(const WeaponDatabase& weaponDB) const;

    // 見つからない場合は nullptr を返す
    const ItemData* Find(ItemID id) const;

    // WeaponID から対応する武器アイテムを逆引きする（ドロップテーブル作成の確認用）。
    // 見つからない場合は nullptr を返す。
    const ItemData* FindByWeaponID(WeaponID weaponID) const;

    const std::vector<const ItemData*>& GetAll() const { return m_Sorted; }

private:
    std::unordered_map<int, ItemData> m_Items;   // キーは ItemID::number
    std::vector<const ItemData*>      m_Sorted;  // ID 昇順のビュー
};
