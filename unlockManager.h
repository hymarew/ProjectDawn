#pragma once
#include <string>

// =====================================================
// UnlockManager : コンテンツ解放状態の一元判定
//
// 「何が解放されているか」の判定をシーンから分離する。
// メニュー側は IsUnlocked(キー) を呼ぶだけでよく、
// 解放の条件・保存方法（現在は save.json の unlocks セクション）を知らない。
//
// 【設計意図】
//   将来 Achievement 連動・ショップ購入・武器強化などの解放条件が
//   増えても、このクラスの中だけで判定ロジックを差し替えられる
//   （シーン側のコードは変わらない = OCP）。
//
// 【キーの規約】
//   save.json の unlocks セクションは Inventory の武器所持
//   ("weapon101" 等) と共用のため、コンテンツ解放キーは
//   衝突しない別プレフィックスにする（例: "storyClear"）。
// =====================================================
namespace UnlockManager
{
    // ストーリーをクリア済みか（EXTRAメニューの解放条件）
    bool IsStoryCleared();

    // ストーリークリアを記録して即セーブする（StoryCompleteScene から呼ぶ）
    void MarkStoryCleared();

    // 汎用の解放判定/記録（将来のショップ・図鑑・武器強化などで使う）
    bool IsUnlocked(const std::string& key);
    void Unlock(const std::string& key);
}
