#include "playerLog.h"
#include <fstream>
#include <iomanip>
#include <ctime>

PlayerLog g_PlayerLog;

void PlayerLog::Reset()
{
    *this = PlayerLog{};
}

static std::string Indent(int n) { return std::string(n * 4, ' '); }

void PlayerLog::Save() const
{
    // 終了日時を取得
    time_t now = time(nullptr);
    struct tm t = {};
    localtime_s(&t, &now);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &t);

    // accuracy 計算
    float acc = (shotsFired > 0) ? (shotsHit / static_cast<float>(shotsFired) * 100.0f) : 0.0f;

    // 武器使用率計算用の合計
    int totalFired = 0;
    for (auto it = weaponShotsFired.begin(); it != weaponShotsFired.end(); ++it)
        totalFired += it->second;

    std::ofstream f("playerlog.json");
    if (!f.is_open()) return;

    f << std::fixed;
    f << "{\n";
    f << Indent(1) << "\"startTime\": \"" << startTime << "\",\n";
    f << Indent(1) << "\"endTime\": \"" << endTime << "\",\n\n";

    f << Indent(1) << "\"maxWave\": " << maxWave << ",\n";
    f << Indent(1) << "\"playTime\": " << std::setprecision(1) << playTime << ",\n\n";

    f << Indent(1) << "\"totalKills\": " << totalKills << ",\n";
    f << Indent(1) << "\"totalDamageTaken\": " << std::setprecision(0) << totalDamageTaken << ",\n\n";

    f << Indent(1) << "\"shotsFired\": " << shotsFired << ",\n";
    f << Indent(1) << "\"shotsHit\": " << shotsHit << ",\n";
    f << Indent(1) << "\"accuracy\": " << std::setprecision(1) << acc << ",\n\n";

    // weaponUsage
    f << Indent(1) << "\"weaponUsage\":\n";
    f << Indent(1) << "{\n";
    {
        bool first = true;
        for (auto it = weaponShotsFired.begin(); it != weaponShotsFired.end(); ++it)
        {
            float pct = (totalFired > 0)
                ? (it->second / static_cast<float>(totalFired) * 100.0f)
                : 0.0f;
            if (!first) f << ",\n";
            f << Indent(2) << "\"" << it->first << "\": " << std::setprecision(1) << pct;
            first = false;
        }
    }
    f << "\n" << Indent(1) << "},\n\n";

    // enemyKills
    f << Indent(1) << "\"enemyKills\":\n";
    f << Indent(1) << "{\n";
    {
        bool first = true;
        for (auto it = enemyKills.begin(); it != enemyKills.end(); ++it)
        {
            if (!first) f << ",\n";
            f << Indent(2) << "\"" << it->first << "\": " << it->second;
            first = false;
        }
    }
    f << "\n" << Indent(1) << "},\n\n";

    // damageSources
    f << Indent(1) << "\"damageSources\":\n";
    f << Indent(1) << "{\n";
    {
        bool first = true;
        for (auto it = damageSources.begin(); it != damageSources.end(); ++it)
        {
            if (!first) f << ",\n";
            f << Indent(2) << "\"" << it->first << "\": " << it->second;
            first = false;
        }
    }
    f << "\n" << Indent(1) << "},\n\n";

    // deathPosition（ゲームクリア時は null）
    if (deathCause == "Clear")
    {
        f << Indent(1) << "\"deathPosition\": null,\n\n";
    }
    else
    {
        f << Indent(1) << "\"deathPosition\":\n";
        f << Indent(1) << "{\n";
        f << Indent(2) << "\"x\": " << std::setprecision(1) << deathPosition.x << ",\n";
        f << Indent(2) << "\"y\": " << std::setprecision(1) << deathPosition.y << ",\n";
        f << Indent(2) << "\"z\": " << std::setprecision(1) << deathPosition.z << "\n";
        f << Indent(1) << "},\n\n";
    }

    f << Indent(1) << "\"deathCause\": \"" << deathCause << "\"\n";
    f << "}\n";
}
