#include "saveManager.h"
#include <fstream>
#include <sstream>
#include <windows.h>  // CreateDirectoryA

SaveManager g_SaveManager;

// ---------------------------------------------------------
// InitDefault : 新規セーブ時のデフォルト値を設定する
// ---------------------------------------------------------
void SaveManager::InitDefault()
{
    m_Data = SaveData{};  // 値代入でマップを空にリセットする

    // Stage1 のみ最初から解放、他はロック
    m_Data.stages["stage1"] = true;
    m_Data.stages["stage2"] = false;
    m_Data.stages["stage3"] = false;

    // achievements / unlocks は空のまま（将来用）
}

// ---------------------------------------------------------
// Load : save.json を読み込む
//
// JSON ライブラリを使わず手動パースする。
// ファイル形式は Save() が出力する固定フォーマットに依存する。
// ---------------------------------------------------------
bool SaveManager::Load()
{
    std::ifstream f(SAVE_PATH);
    if (!f.is_open())
    {
        // ファイルなし → デフォルトデータを生成して保存する
        InitDefault();
        return Save();
    }

    // ファイルを全文読み込む
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    // どのセクションを読んでいるかを追跡する。
    // "stages" / "achievements" / "unlocks" のいずれかを設定する。
    std::string section;

    std::istringstream lines(src);
    std::string line;
    while (std::getline(lines, line))
    {
        // セクション検出："stages" のように先頭に来る
        auto detectSection = [&](const char* name) {
            return line.find(std::string("\"") + name + "\"") != std::string::npos;
        };
        if      (detectSection("stages"))       section = "stages";
        else if (detectSection("achievements")) section = "achievements";
        else if (detectSection("unlocks"))      section = "unlocks";

        // "key": true/false パターンを探す
        // 行例: `        "stage1": true,`
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        // コロン左側からキーを抽出。
        // コロン直前の " を q1、その前の " を q0 として両端を特定する。
        // q0 == q1 は空文字列キーを弾く条件。
        auto q1 = line.rfind('"', colon);
        auto q0 = (q1 != std::string::npos) ? line.rfind('"', q1 - 1) : std::string::npos;
        if (q0 == std::string::npos || q1 == std::string::npos || q0 == q1) continue;

        std::string key = line.substr(q0 + 1, q1 - q0 - 1);

        // コロン右側から true / false を検出
        std::string rhs = line.substr(colon + 1);
        bool hasTrue  = rhs.find("true")  != std::string::npos;
        bool hasFalse = rhs.find("false") != std::string::npos;
        if (!hasTrue && !hasFalse) continue;

        // セクション名自体の行（例: `"stages":` ）はキーとして誤検出されるのでスキップする
        if (key == section) continue;

        // true と false が両方含まれる行はありえないが、安全のため hasTrue 優先とする
        bool value = hasTrue;

        if      (section == "stages")       m_Data.stages[key]       = value;
        else if (section == "achievements") m_Data.achievements[key] = value;
        else if (section == "unlocks")      m_Data.unlocks[key]      = value;
    }

    return true;
}

// ---------------------------------------------------------
// Save : m_Data を save.json に書き出す
// ---------------------------------------------------------
bool SaveManager::Save()
{
    // Data/ フォルダが存在しない場合は作成する。
    // 既に存在していても ERROR_ALREADY_EXISTS を返すだけで失敗しないため戻り値は無視する。
    CreateDirectoryA("Data", nullptr);

    std::ofstream f(SAVE_PATH);
    if (!f.is_open()) return false;

    auto Indent = [](int n) { return std::string(n * 4, ' '); };

    // bool を JSON リテラルに変換するヘルパー
    auto BoolStr = [](bool v) -> const char* { return v ? "true" : "false"; };

    f << "{\n";

    // ---- stages ----
    f << Indent(1) << "\"stages\":\n";
    f << Indent(1) << "{\n";
    {
        bool first = true;
        for (const auto& kv : m_Data.stages)
        {
            if (!first) f << ",\n";
            f << Indent(2) << "\"" << kv.first << "\": " << BoolStr(kv.second);
            first = false;
        }
    }
    f << "\n" << Indent(1) << "},\n\n";

    // ---- achievements ----
    f << Indent(1) << "\"achievements\":\n";
    f << Indent(1) << "{\n";
    {
        bool first = true;
        for (const auto& kv : m_Data.achievements)
        {
            if (!first) f << ",\n";
            f << Indent(2) << "\"" << kv.first << "\": " << BoolStr(kv.second);
            first = false;
        }
    }
    f << "\n" << Indent(1) << "},\n\n";

    // ---- unlocks ----
    f << Indent(1) << "\"unlocks\":\n";
    f << Indent(1) << "{\n";
    {
        bool first = true;
        for (const auto& kv : m_Data.unlocks)
        {
            if (!first) f << ",\n";
            f << Indent(2) << "\"" << kv.first << "\": " << BoolStr(kv.second);
            first = false;
        }
    }
    f << "\n" << Indent(1) << "}\n";

    f << "}\n";

    // f.good() で書き込みエラーをまとめて検出する。
    // ofstream はデストラクタで flush されるため、ここで状態を確認する。
    return f.good();
}

// ---------------------------------------------------------
// IsStageUnlocked / SetStageUnlocked
// StageDatabase から呼ばれる解放状態のアクセサ。
// キーが存在しない場合は安全なデフォルト値を返す。
// ---------------------------------------------------------
bool SaveManager::IsStageUnlocked(const std::string& stageId) const
{
    auto it = m_Data.stages.find(stageId);
    // キーが存在しない場合は未解放扱い
    return (it != m_Data.stages.end()) ? it->second : false;
}

void SaveManager::SetStageUnlocked(const std::string& stageId, bool unlocked)
{
    // メモリ上の値を更新するだけで Save() は呼ばない。
    // Save() は呼び出し元（StageDatabase::UnlockStage）が責任を持って呼ぶ設計にすることで、
    // 複数フィールドをまとめて更新してから1度だけ保存するパターンに対応できる。
    m_Data.stages[stageId] = unlocked;
}
