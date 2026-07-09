#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

// =====================================================
// FullScreenQuad : 白色1×1テクスチャを画面全体に拡大表示する共通描画ヘルパー
//
// 色とアルファを変えるだけで黒フェード・白フェード・画面フラッシュなど
// あらゆる単色オーバーレイに使い回せる（描画処理を統一するのが狙い）。
// Transition専用にはせず、ダメージ演出等からも再利用できるよう独立させている。
//
// テクスチャはファイルを読まず、コード上で1×1の白ピクセルを生成する。
// =====================================================
class FullScreenQuad
{
public:
    void Init();
    void Uninit();

    // color : RGBA。1×1の白テクスチャに乗算されるのでそのまま最終色になる
    // world : 矩形のワールド変換（Slide/Curtain等、位置をずらしたい場合に指定する）
    void Draw(const XMFLOAT4& color, const XMMATRIX& world = XMMatrixIdentity()) const;

    // Wipe/Circle/Mosaic等、独自ピクセルシェーダーを使うTransition用。
    // 呼び出し側で VSSetShader ... PSSetShader / 定数バッファ / テクスチャを設定した後にこれを呼ぶ。
    // 頂点シェーダーは共通のパススルー(unlitTextureVS)を毎回セットする。
    void DrawGeometry() const;

private:
    ID3D11Buffer*             m_VertexBuffer = nullptr;
    ID3D11InputLayout*        m_VertexLayout = nullptr;
    ID3D11VertexShader*       m_VertexShader = nullptr;
    ID3D11PixelShader*        m_PixelShader  = nullptr;
    ID3D11ShaderResourceView* m_WhiteTexture = nullptr; // 1×1 白ピクセル
};
