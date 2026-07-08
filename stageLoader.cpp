#include "stageLoader.h"
#include <fstream>
#include <sstream>

// ---------------------------------------------------------
// ParseString : src から `"key": "value"` を探して value を返す
//
// JSON ライブラリを使わず手動パースする（playerLog.cpp / saveManager.cpp と同方針）。
// value 側の最初の " ～ 次の " を取り出すだけなので、値内にエスケープを含む場合は非対応。
// ---------------------------------------------------------
bool StageLoader::ParseString(const std::string& src,
                              const std::string& key,
                              std::string& outValue)
{
    // キーを `"key":` の形で検索する
    std::string token = "\"" + key + "\"";
    auto pos = src.find(token);
    if (pos == std::string::npos) return false;

    // コロンの次から値の開き " を探す
    auto colon = src.find(':', pos + token.size());
    if (colon == std::string::npos) return false;

    // vq0 = 値の開き "、vq1 = 値の閉じ "。その間の文字列を抽出する。
    auto vq0 = src.find('"', colon + 1);
    if (vq0 == std::string::npos) return false;

    auto vq1 = src.find('"', vq0 + 1);
    if (vq1 == std::string::npos) return false;

    outValue = src.substr(vq0 + 1, vq1 - vq0 - 1);
    return true;
}

// ---------------------------------------------------------
// ParseInt : src から `"key": number` を探して int を返す
// ---------------------------------------------------------
bool StageLoader::ParseInt(const std::string& src,
                           const std::string& key,
                           int& outValue)
{
    std::string token = "\"" + key + "\"";
    auto pos = src.find(token);
    if (pos == std::string::npos) return false;

    auto colon = src.find(':', pos + token.size());
    if (colon == std::string::npos) return false;

    // コロンの後ろで最初の数字を探す
    auto numPos = src.find_first_of("0123456789", colon + 1);
    if (numPos == std::string::npos) return false;

    // stoi は先頭から数字が続く範囲だけを読むので、
    // 後続の `,` や `}` は自動的に無視される
    outValue = std::stoi(src.substr(numPos));
    return true;
}

// ---------------------------------------------------------
// Load : JSON ファイルを読み込んで StageData を生成する
// ---------------------------------------------------------
bool StageLoader::Load(const std::string& path, StageData& outStage)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;  // ファイル不在

    // ファイル全体を一度に読み込む
    std::ostringstream ss;
    ss << f.rdbuf();
    const std::string src = ss.str();

    // ---- id ----
    {
        std::string idStr;
        if (!ParseString(src, "id", idStr)) return false;

        StageID sid;
        if (!KeyToStageID(idStr, sid)) return false;  // 未知の id は不正ファイル扱い
        outStage.id = sid;
    }

    // ---- name ----
    if (!ParseString(src, "name", outStage.name)) return false;

    // ---- description ----
    if (!ParseString(src, "description", outStage.description)) return false;

    // ---- thumbnail ----
    // 読み込み失敗してもゲーム進行に影響しないため、失敗は無視する
    ParseString(src, "thumbnail", outStage.thumbnailPath);

    // ---- waves ----
    // "waves": [ { "spawnerCount": N }, ... ] を手動で走査する。
    // waves 配列の開き '[' から閉じ ']' までを切り出して、
    // その範囲内で "spawnerCount" を繰り返し検索する。
    outStage.waves.clear();
    {
        auto wpos = src.find("\"waves\"");
        if (wpos != std::string::npos)
        {
            auto arrOpen  = src.find('[', wpos);
            auto arrClose = src.find(']', wpos);

            // arrOpen < arrClose で "[" が "]" より前にあることを保証し、
            // 壊れた JSON で arrClose が arrOpen より前に来るケースを弾く
            if (arrOpen != std::string::npos && arrClose != std::string::npos
                && arrOpen < arrClose)
            {
                std::string arrSrc = src.substr(arrOpen, arrClose - arrOpen + 1);

                // 配列内のオブジェクト '{' ... '}' を1つずつ処理する
                std::string::size_type cur = 0;
                while (true)
                {
                    auto objOpen  = arrSrc.find('{', cur);
                    auto objClose = arrSrc.find('}', cur);
                    if (objOpen  == std::string::npos) break;
                    if (objClose == std::string::npos) break;

                    std::string objSrc = arrSrc.substr(objOpen, objClose - objOpen + 1);

                    WaveData wave;
                    // spawnerCount が取れなければ 0 のまま追加する
                    ParseInt(objSrc, "spawnerCount", wave.spawnerCount);

                    // ---- 直接生成方式（enemyCount指定時のみ意味を持つ） ----
                    ParseInt(objSrc, "enemyCount", wave.enemyCount);

                    // aiState: "patrol" なら Idle から開始、それ以外（省略時含む）は Active
                    std::string aiState;
                    if (ParseString(objSrc, "aiState", aiState))
                        wave.startActive = (aiState != "patrol");

                    int sx = 0, sy = 0, sz = 0;
                    ParseInt(objSrc, "spawnX", sx);
                    ParseInt(objSrc, "spawnY", sy);
                    ParseInt(objSrc, "spawnZ", sz);
                    wave.spawnPos = { (float)sx, (float)sy, (float)sz };

                    outStage.waves.push_back(wave);

                    cur = objClose + 1;  // 次のオブジェクト検索を '}' の直後から始める
                }
            }
        }
    }

    return true;
}
