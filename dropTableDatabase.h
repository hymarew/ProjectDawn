#pragma once
#include <unordered_map>
#include <string>
#include "dropTableData.h"

class ItemDatabase;

// =====================================================
// DropTableDatabase : ドロップテーブルの読み込みと検索
//
// Data/DropTables/*.json を走査して構築する。
// 検索キーは敵の TypeName（Enemy::GetTypeName() の戻り値）。
// テーブルが存在しない敵は「何もドロップしない」扱いになる。
// =====================================================
class DropTableDatabase
{
public:
    bool Load();

    // entries の itemID が ItemDatabase に存在するか検証する。
    // 参照切れはログへ警告を出す。戻り値: 参照切れなしなら true
    bool Validate(const ItemDatabase& itemDB) const;

    // 見つからない場合は nullptr を返す（＝ドロップなし）
    const DropTableData* Find(const std::string& enemyTypeName) const;

private:
    std::unordered_map<std::string, DropTableData> m_Tables;  // キーは enemyTypeName
};
