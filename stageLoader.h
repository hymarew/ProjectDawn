#pragma once
#include <string>
#include "stageData.h"

// =====================================================
// StageLoader : ステージ定義 JSON を読み込んで StageData を生成する
//
// StageDatabase とは独立して動作する（Step4-3A 時点）。
// StageDatabase への接続は次ステップで実装する。
// =====================================================
class StageLoader
{
public:
    // path の JSON を読み込み outStage に結果を書き込む。
    // 戻り値: 成功 = true、ファイル不在 or 形式不正 = false
    bool Load(const std::string& path, StageData& outStage);

private:
    // JSON 文字列から "key": "value" を取り出すユーティリティ
    static bool ParseString(const std::string& src,
                            const std::string& key,
                            std::string& outValue);

    // JSON 文字列から "key": number を取り出すユーティリティ
    static bool ParseInt(const std::string& src,
                         const std::string& key,
                         int& outValue);
};
