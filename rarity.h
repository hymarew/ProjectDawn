#pragma once
#include <string>

// =====================================================
// Rarity : アイテム・武器のレアリティ
//
// JSON では "Common" のような文字列で書き、ロード時に enum へ変換する。
// UI 表示色・ワールドドロップの発光色もここで一元管理する。
// =====================================================
enum class Rarity
{
    Common = 0,   // 白
    Rare,         // 青
    Epic,         // 紫
    Legendary,    // 橙
};

// UI・発光用の色（0.0〜1.0）
struct RarityColor
{
    float r, g, b;
};

inline const char* RarityToString(Rarity rarity)
{
    switch (rarity)
    {
    case Rarity::Common:    return "Common";
    case Rarity::Rare:      return "Rare";
    case Rarity::Epic:      return "Epic";
    case Rarity::Legendary: return "Legendary";
    }
    return "Common";
}

// 未知の文字列は Common 扱いにして false を返す（ロード時警告用）
inline bool RarityFromString(const std::string& str, Rarity& outRarity)
{
    if (str == "Common")    { outRarity = Rarity::Common;    return true; }
    if (str == "Rare")      { outRarity = Rarity::Rare;      return true; }
    if (str == "Epic")      { outRarity = Rarity::Epic;      return true; }
    if (str == "Legendary") { outRarity = Rarity::Legendary; return true; }
    outRarity = Rarity::Common;
    return false;
}

inline RarityColor GetRarityColor(Rarity rarity)
{
    switch (rarity)
    {
    case Rarity::Common:    return { 0.85f, 0.85f, 0.85f }; // 白
    case Rarity::Rare:      return { 0.30f, 0.60f, 1.00f }; // 青
    case Rarity::Epic:      return { 0.75f, 0.35f, 1.00f }; // 紫
    case Rarity::Legendary: return { 1.00f, 0.60f, 0.10f }; // 橙
    }
    return { 1.0f, 1.0f, 1.0f };
}
