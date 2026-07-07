#pragma once

// ゲームモードの種別。GameContext に保持し、各シーンが参照する。
enum class GameMode
{
    Story,    // ストーリーモード（Wave クリアで終了）
    Endless,  // エンドレスモード（Wave が無限に続く）
};
