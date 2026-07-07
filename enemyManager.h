#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

class EnemyPool;

// GPUに1体分として送るデータ構造（シェーダー側のレイアウトと合わせること）
struct InstanceData
{
    XMFLOAT4X4 worldMatrix; // ワールド行列（位置・回転・スケールをまとめたもの）
};

// EnemyManagerの責務:
//   EnemyPoolからアクティブな敵のデータを受け取り、GPUへ転送・描画する。
//   描画に必要なシェーダーやインスタンスバッファはこのクラスが所有する。
//   敵の種類が増えたときは、このクラスにタイプ別の描画ロジックを追加する。
class EnemyManager
{
private:
    ID3D11Buffer*       m_InstanceBuffer = nullptr; // アクティブな敵の行列を毎フレームGPUに転送するバッファ
    ID3D11VertexShader* m_VertexShader   = nullptr; // インスタンシング専用の頂点シェーダー
    ID3D11PixelShader*  m_PixelShader    = nullptr; // ピクセルシェーダー（シャドウマップ対応）
    ID3D11InputLayout*  m_InputLayout    = nullptr; // 頂点レイアウト（モデル頂点＋インスタンスデータの2スロット構成）

public:
    EnemyManager() {}
    ~EnemyManager() {}

    // インスタンスバッファとシェーダーを初期化する。maxEnemiesはEnemyPoolと同じ値を渡す
    void Init(int maxEnemies);

    // GPUリソースを解放する
    void Uninit();

    // poolのアクティブな敵をインスタンシングで一括描画する
    void Draw(EnemyPool& pool);

    // poolのアクティブな敵の影を描画する
    void DrawShadow(EnemyPool& pool);
};
