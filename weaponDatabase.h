#pragma once
#include <vector>
#include <unordered_map>
#include "weaponData.h"

// =====================================================
// WeaponDatabase : 武器マスタデータの読み込みと検索
//
// Data/Weapons/*.json を走査して構築する（StageDatabase と同方式）。
// 読み取り専用の最下層クラスで、他のクラスには依存しない。
// 武器を増やすときは JSON を追加するだけでよい。
// =====================================================
class WeaponDatabase
{
public:
    // Data/Weapons/*.json を読み込む。壊れたファイルはログを出してスキップする。
    // ID 重複はロード時に検出して警告し、後勝ちで上書きする。
    // 戻り値: 1件以上読み込めたら true
    bool Load();

    // 見つからない場合は nullptr を返す
    const WeaponData* Find(WeaponID id) const;

    // ID 昇順の一覧（WeaponSelect 画面などの表示用）
    const std::vector<const WeaponData*>& GetAll() const { return m_Sorted; }

private:
    std::unordered_map<int, WeaponData> m_Weapons;  // キーは WeaponID::number
    std::vector<const WeaponData*>      m_Sorted;   // ID 昇順のビュー
};
