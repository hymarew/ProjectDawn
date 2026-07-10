#pragma once
#include <d3d11.h>
#include <unordered_map>
#include <string>
#include <vector>
#include "particleSetting.h"

// パーティクルの GPU 描画を一手に担うクラス
//
// 【GPUインスタンシング方式】
//   旧実装は「1パーティクル = 1ドローコール + 定数バッファ更新×2」だったため、
//   パーティクル数がそのままCPU負荷になっていた。
//   本実装では全パーティクルのインスタンスデータ（位置・サイズ・色・回転）を
//   毎フレーム1回のMapでGPUへ転送し、(テクスチャ × ブレンド種別) のグループごとに
//   DrawInstanced を1回発行するだけで済む（ドローコールは最大でも数回）。
class ParticleRenderer
{
public:
    // maxInstances: インスタンスバッファの容量（= パーティクルプールの上限）
    void Init(int maxInstances);
    void Uninit();

    // pool 内のアクティブなパーティクルをすべて描画する
    void Draw(const ParticleData* pool, int poolSize);

    // 直近フレームの統計（デバッグUI表示用）
    int GetActiveCount()   const { return m_ActiveCount; }
    int GetDrawCallCount() const { return m_DrawCalls; }

private:
    // 1パーティクル分のインスタンスデータ（GPUへ毎フレーム転送する。48バイト）
    // particleVS.hlsl の TEXCOORD1〜3 と同じレイアウトにすること
    struct ParticleInstance
    {
        XMFLOAT3 Position;      // ワールド位置
        float    Size;          // 現在サイズ（補間済み）
        XMFLOAT4 Color;         // 現在色（アルファ含む）
        float    Rotation;      // 面内スピン角（ラジアン）
        float    GroundAligned; // 0=ビルボード / 1=地面水平
        XMFLOAT2 Pad;
    };

    // ドローコール1回分のまとまり（同じテクスチャ・同じブレンドのパーティクル群）
    struct Bucket
    {
        ID3D11ShaderResourceView*     Texture;
        bool                          Additive;
        std::vector<ParticleInstance> Instances;
    };

    // テクスチャキャッシュ（同一パスの二重ロードを防ぐ）
    ID3D11ShaderResourceView* GetOrLoadTexture(const wchar_t* path);

    // 該当する (テクスチャ, ブレンド) のバケットを探し、なければ作る
    Bucket& FindOrAddBucket(ID3D11ShaderResourceView* texture, bool additive);

    ID3D11Buffer*       m_QuadVertexBuffer = nullptr; // 全パーティクル共有の板ポリ（4頂点）
    ID3D11Buffer*       m_InstanceBuffer   = nullptr; // インスタンスデータ（DYNAMIC）
    int                 m_InstanceCapacity = 0;       // インスタンスバッファの容量
    ID3D11InputLayout*  m_InputLayout      = nullptr;
    ID3D11VertexShader* m_VertexShader     = nullptr;
    ID3D11PixelShader*  m_PixelShader      = nullptr;

    std::vector<Bucket> m_Buckets; // 毎フレーム使い回す（capacityを保持して再確保を防ぐ）

    std::unordered_map<std::wstring, ID3D11ShaderResourceView*> m_TextureCache;

    // デフォルトテクスチャ（プリセット未指定時のフォールバック）
    ID3D11ShaderResourceView* m_DefaultTexture = nullptr;

    // 統計（デバッグUI表示用）
    int m_ActiveCount = 0;
    int m_DrawCalls   = 0;
};
