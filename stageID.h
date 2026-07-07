#pragma once
#include <string>

// =====================================================
// StageID : ステージの識別子
//
// "stage1", "stage2", "stage11" のように連番の文字列で管理されるため、
// StageID は個別に名前を付けた列挙値ではなく、番号そのものを保持する
// 軽量な値型にしている。ステージが増えても定義を追加する必要がない。
//
// GameContext::currentStage に保持し StageSelectScene → GameScene へ受け渡す。
// =====================================================
struct StageID
{
    int number = 0;

    constexpr StageID() = default;
    constexpr explicit StageID(int n) : number(n) {}

    constexpr bool operator==(const StageID& other) const { return number == other.number; }
    constexpr bool operator!=(const StageID& other) const { return number != other.number; }
    constexpr bool operator<(const StageID& other)  const { return number <  other.number; }
};

// StageID(1) → "stage1"（SaveManager のキーや JSON の id と対応する）
std::string StageIDToKey(StageID id);

// "stage1" → StageID(1)。"stage" + 数字 の形式でなければ false を返す。
bool KeyToStageID(const std::string& key, StageID& outID);
