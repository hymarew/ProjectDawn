#pragma once
#include <vector>
#include "weaponData.h"

class Inventory;

// =====================================================
// PendingWeapons : ステージ中のみ保持する仮取得武器リスト
//
// 敵ドロップから拾った武器はまずここに入り、
// ステージクリア時に CommitTo() で Inventory へ正式取得される。
// ゲームオーバー・リタイア・タイトル戻りでは Discard() で失われる。
//
// このクラスは一切セーブしない（メモリのみ）。
// 「クリア時だけ save.json に書かれる」ため、強制終了を含む
// あらゆる途中終了で仮取得が自然に失われ、破棄漏れが起きない。
//
// 寿命はステージ1回分。GameScene が所有する。
// =====================================================
class PendingWeapons
{
public:
    enum class AddResult
    {
        Pending,         // 仮取得した（クリアで正式取得）
        AlreadyOwned,    // Inventory に所持済み（追加しない）
        AlreadyPending,  // このステージで仮取得済み（追加しない）
    };

    // ItemPickupSystem から呼ばれる。所持済み・仮取得済みは弾く
    AddResult Add(WeaponID id, const Inventory& inventory);

    // ステージクリア時に1回だけ呼ぶ。全件を Inventory へ正式取得して空にする。
    // Inventory::AddWeapon が1件ごとに save.json へ即時保存する。
    void CommitTo(Inventory& inventory);

    // 途中終了時に呼ぶ。全件破棄（クリア時は Commit 済みで空なので無害）
    void Discard() { m_Pending.clear(); }

    int  GetCount() const { return (int)m_Pending.size(); }
    bool Contains(WeaponID id) const;

    // HUD「Found: N」やリザルトでの獲得一覧表示用
    const std::vector<WeaponID>& GetAll() const { return m_Pending; }

private:
    std::vector<WeaponID> m_Pending;
};
