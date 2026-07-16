#pragma once
#include <vector>
#include "dropTableData.h"

// =====================================================
// IDropStrategy : ドロップ抽選アルゴリズムのインターフェース（Strategy パターン）
//
// 「DropTableData を受け取って、ドロップする ItemID の列を返す」だけの
// 純粋な抽選ロジック。アイテムの生成（ItemFactory 呼び出し）は
// DropManager 側の仕事なので持たない。
//
// 敵ランク追加で新しい抽選方法が必要になったら
//   1. 実装クラスを1つ作る
//   2. DropManager の登録表に strategyType 文字列と対応付けて1行追加する
// =====================================================
class IDropStrategy
{
public:
    virtual ~IDropStrategy() = default;

    // table の内容で抽選し、ドロップする ItemID を outItems へ追加する
    // （ドロップなしのときは何も追加しない）
    virtual void Select(const DropTableData& table, std::vector<ItemID>& outItems) = 0;

protected:
    // 重み付きランダムで entries から1件選ぶ共通処理。
    // entries が空のときは無効な ItemID を返す。
    static ItemID PickWeighted(const std::vector<DropEntry>& entries);
};
