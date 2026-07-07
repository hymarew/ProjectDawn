#pragma once
#include <d3d11.h>
#include <unordered_map>
#include <string>
#include "particleSetting.h"

// パーティクルの GPU 描画を一手に担うクラス
// ParticleManager から Draw() を呼ばれ、アクティブなパーティクルをビルボード方式で描画する
// 描画ロジックをここに集約することで、将来のインスタンシング対応がこのクラスだけで完結する
class ParticleRenderer
{
public:
    void Init();
    void Uninit();

    // pool 内のアクティブなパーティクルをすべて描画する
    void Draw(const ParticleData* pool, int poolSize);

private:
    // テクスチャキャッシュ（同一パスの二重ロードを防ぐ）
    ID3D11ShaderResourceView* GetOrLoadTexture(const wchar_t* path);

    ID3D11Buffer*             m_VertexBuffer;
    ID3D11InputLayout*        m_VertexLayout;
    ID3D11VertexShader*       m_VertexShader;
    ID3D11PixelShader*        m_PixelShader;

    std::unordered_map<std::wstring, ID3D11ShaderResourceView*> m_TextureCache;

    // デフォルトテクスチャ（プリセット未指定時のフォールバック）
    ID3D11ShaderResourceView* m_DefaultTexture = nullptr;
};
