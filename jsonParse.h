#pragma once
#include <string>
#include <functional>

// =====================================================
// JsonParse : JSON 手動パースの共通ユーティリティ
//
// StageLoader / SaveManager と同方針で JSON ライブラリを使わず、
// `"key": value` を文字列検索で取り出す。
// WeaponDatabase / ItemDatabase / DropTableDatabase のローダーが共用する。
//
// 制約（既存ローダーと同じ）:
//   - 値内のエスケープ文字列は非対応
//   - 同名キーが複数あるとき最初の1つだけを読む
// =====================================================
namespace JsonParse
{
    // `"key": "value"` から value を取り出す
    bool ParseString(const std::string& src, const std::string& key, std::string& outValue);

    // `"key": 123` から int を取り出す（負数対応）
    bool ParseInt(const std::string& src, const std::string& key, int& outValue);

    // `"key": 1.5` から float を取り出す（負数・小数対応）
    bool ParseFloat(const std::string& src, const std::string& key, float& outValue);

    // `"key": true/false` から bool を取り出す
    bool ParseBool(const std::string& src, const std::string& key, bool& outValue);

    // `"key": [ {...}, {...} ]` の配列内オブジェクトを1つずつ callback へ渡す。
    // ネストした {} は非対応（StageLoader の waves 走査と同じ制約）。
    // 配列キーが見つからなければ false を返す（空配列は true）。
    bool ForEachArrayObject(const std::string& src, const std::string& key,
                            const std::function<void(const std::string& objSrc)>& callback);
}
