#pragma once
#include <vector>

class WorldItem;

// =====================================================
// WorldItemPool : WorldItem を事前確保するオブジェクトプール
//
// Wave 制で数十体同時死亡 → 大量ドロップが起きるため、
// BulletPool / EnemyPool と同じ「事前確保 + active フラグ」方式で
// 実行中の new/delete をなくしフレームスパイクを防ぐ。
//
// WorldItem は Manager の GameObject リストには入れない。
// Update/Draw は gameScene.cpp から直接呼ばれる。
// =====================================================
class WorldItemPool
{
public:
    void Init(int maxCount);
    void Uninit();
    void Update(float dt);
    void Draw();
    void DrawShadow();

    // 空きスロットを返す。満杯なら nullptr（呼び出し側でドロップを諦める）
    WorldItem* Acquire();

    int GetActiveCount() const;

    std::vector<WorldItem*>& GetItems() { return m_Pool; }

private:
    std::vector<WorldItem*> m_Pool;
};

extern WorldItemPool g_WorldItemPool;
