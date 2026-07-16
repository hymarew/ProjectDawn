#include "jsonParse.h"

namespace
{
    // `"key":` を探してコロンの位置を返す。見つからなければ npos。
    std::string::size_type FindValueStart(const std::string& src, const std::string& key)
    {
        std::string token = "\"" + key + "\"";
        auto pos = src.find(token);
        if (pos == std::string::npos) return std::string::npos;

        return src.find(':', pos + token.size());
    }
}

namespace JsonParse
{

bool ParseString(const std::string& src, const std::string& key, std::string& outValue)
{
    auto colon = FindValueStart(src, key);
    if (colon == std::string::npos) return false;

    // 値の開き " ～ 閉じ " を取り出す
    auto vq0 = src.find('"', colon + 1);
    if (vq0 == std::string::npos) return false;

    auto vq1 = src.find('"', vq0 + 1);
    if (vq1 == std::string::npos) return false;

    outValue = src.substr(vq0 + 1, vq1 - vq0 - 1);
    return true;
}

bool ParseInt(const std::string& src, const std::string& key, int& outValue)
{
    auto colon = FindValueStart(src, key);
    if (colon == std::string::npos) return false;

    // 負数に対応するため '-' も数値の開始文字として扱う
    auto numPos = src.find_first_of("-0123456789", colon + 1);
    if (numPos == std::string::npos) return false;

    // stoi は先頭から数字が続く範囲だけを読むので後続の `,` `}` は無視される
    outValue = std::stoi(src.substr(numPos));
    return true;
}

bool ParseFloat(const std::string& src, const std::string& key, float& outValue)
{
    auto colon = FindValueStart(src, key);
    if (colon == std::string::npos) return false;

    auto numPos = src.find_first_of("-0123456789.", colon + 1);
    if (numPos == std::string::npos) return false;

    outValue = std::stof(src.substr(numPos));
    return true;
}

bool ParseBool(const std::string& src, const std::string& key, bool& outValue)
{
    auto colon = FindValueStart(src, key);
    if (colon == std::string::npos) return false;

    // コロン直後から次のカンマ/閉じ括弧までに true/false があるか調べる
    auto end = src.find_first_of(",}", colon + 1);
    std::string rhs = src.substr(colon + 1,
        (end == std::string::npos) ? std::string::npos : end - colon - 1);

    if (rhs.find("true")  != std::string::npos) { outValue = true;  return true; }
    if (rhs.find("false") != std::string::npos) { outValue = false; return true; }
    return false;
}

bool ForEachArrayObject(const std::string& src, const std::string& key,
                        const std::function<void(const std::string& objSrc)>& callback)
{
    auto keyPos = src.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return false;

    auto arrOpen  = src.find('[', keyPos);
    auto arrClose = src.find(']', keyPos);

    // "[" が "]" より前にあることを保証し、壊れた JSON を弾く
    if (arrOpen == std::string::npos || arrClose == std::string::npos || arrOpen >= arrClose)
        return false;

    std::string arrSrc = src.substr(arrOpen, arrClose - arrOpen + 1);

    // 配列内のオブジェクト '{' ... '}' を1つずつ切り出して渡す
    std::string::size_type cur = 0;
    while (true)
    {
        auto objOpen  = arrSrc.find('{', cur);
        auto objClose = arrSrc.find('}', cur);
        if (objOpen == std::string::npos || objClose == std::string::npos) break;

        callback(arrSrc.substr(objOpen, objClose - objOpen + 1));
        cur = objClose + 1;
    }
    return true;
}

} // namespace JsonParse
