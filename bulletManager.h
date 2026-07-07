#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

class BulletPool;

// =====================================================
// BulletManager : 弾の描画専用クラス
// =====================================================
// EnemyManager と同じ責務分離の設計。
// BulletPool がデータ（位置・速度）を管理し、
// BulletManager が GPU リソース（シェーダー等）を持って描画する。
//
// 弾の数は敵より少ないため、インスタンシングは使わず
// アクティブな弾を1体ずつ Draw している。
class BulletManager
{
private:
    ID3D11InputLayout*  m_InputLayout  = nullptr; // 頂点レイアウト（どんなデータがあるかの定義）
    ID3D11VertexShader* m_VertexShader = nullptr; // 頂点シェーダー（座標変換を行うGPUプログラム）
    ID3D11PixelShader*  m_PixelShader  = nullptr; // ピクセルシェーダー（色の計算を行うGPUプログラム）

public:
    BulletManager()  {}
    ~BulletManager() {}

    // シェーダーを読み込み、bullet.obj モデルをキャッシュに登録する
    void Init();

    // GPU リソースを解放する
    void Uninit();

    // プール内のアクティブな弾を1体ずつ描画する
    void Draw(BulletPool& pool);

    // 影パス用。同じく1体ずつ描画する
    void DrawShadow(BulletPool& pool);
};
