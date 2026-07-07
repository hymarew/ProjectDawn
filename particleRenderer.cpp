// ===================================================
// particleRenderer.cpp
// パーティクルのGPU描画をすべて担当するクラス
//
// 【役割】
//   ParticleManager から全パーティクルのデータを受け取り、
//   ビルボード方式でまとめて描画する。
//
// 【ビルボードとは】
//   板ポリゴンが常にカメラの方を向く技術。
//   木や煙、パーティクルなどに使われる。
//
// 【テクスチャキャッシュについて】
//   同じパスのテクスチャを何度もロードしないよう、
//   一度ロードしたテクスチャを map に保存して使いまわす。
// ===================================================

#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "camera.h"
#include "particleRenderer.h"
#include "DirectXTex.h"

// ---------------------------------------------------------
// Init : GPUリソースの確保とデフォルトテクスチャの読み込み
// ---------------------------------------------------------
void ParticleRenderer::Init()
{
    // ---- 板ポリゴンの頂点データを作る ----
    // 中心を原点とした1×1の四角形。
    // 全パーティクルがこの1枚の板を共有し、ワールド行列でサイズと位置を変える。
    VERTEX_3D vertex[4];
    //              座標(X, Y, Z)         法線(X, Y, Z)      色(R,G,B,A)     UV座標(U, V)
    vertex[0] = { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) }; // 左上
    vertex[1] = { XMFLOAT3( 0.5f,  0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) }; // 右上
    vertex[2] = { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) }; // 左下
    vertex[3] = { XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) }; // 右下

    // ---- 頂点バッファをGPUに作成 ----
    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(VERTEX_3D) * 4;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;

    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

    // ---- シェーダーの読み込み ----
    // ライティングなしでテクスチャをそのまま表示する unlit シェーダーを使用
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso");
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\unlitTexturePS.cso");

    // ---- デフォルトテクスチャを読み込んでキャッシュに登録 ----
    // 他のテクスチャの読み込みに失敗したときのフォールバックとして使う
    m_DefaultTexture = GetOrLoadTexture(L"asset\\texture\\particle.png");
}

// ---------------------------------------------------------
// Uninit : GPUリソースの解放
// ---------------------------------------------------------
void ParticleRenderer::Uninit()
{
    m_VertexBuffer->Release();
    m_VertexLayout->Release();
    m_VertexShader->Release();
    m_PixelShader->Release();

    // テクスチャキャッシュの全エントリを解放
    for (auto& entry : m_TextureCache)
    {
        if (entry.second) entry.second->Release();
    }
    m_TextureCache.clear();
}

// ---------------------------------------------------------
// GetOrLoadTexture : テクスチャをキャッシュ付きで取得
// ---------------------------------------------------------
ID3D11ShaderResourceView* ParticleRenderer::GetOrLoadTexture(const wchar_t* path)
{
    std::wstring key(path);

    // ---- キャッシュを検索 ----
    // 同じパスのテクスチャがすでに読み込まれていたらそれを返す（二重ロード防止）
    auto it = m_TextureCache.find(key);
    if (it != m_TextureCache.end())
        return it->second;

    // ---- テクスチャをファイルから読み込む ----
    TexMetadata  metadata;
    ScratchImage image;
    HRESULT hr = LoadFromWICFile(path, WIC_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
    {
        // 読み込み失敗時はデフォルトテクスチャで代替
        // （クラッシュせずに処理を続けられる）
        return m_DefaultTexture;
    }

    // ---- GPU がシェーダーで読めるビュー（SRV）を作成してキャッシュに登録 ----
    ID3D11ShaderResourceView* srv = nullptr;
    CreateShaderResourceView(Renderer::GetDevice(),
        image.GetImages(), image.GetImageCount(), metadata, &srv);

    m_TextureCache[key] = srv; // 次回以降はファイルを読まずキャッシュから返す
    return srv;
}

// ---------------------------------------------------------
// Draw : アクティブな全パーティクルをビルボード描画
// ---------------------------------------------------------
void ParticleRenderer::Draw(const ParticleData* pool, int poolSize)
{
    // ---- シェーダーのセット ----
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    // ---- ビルボード行列の計算 ----
    // ビュー行列の逆行列から平行移動を除いた「回転だけの行列」を作る。
    // これをワールド行列に使うと、板ポリゴンが常にカメラを向く。
    Camera*  camera  = Manager::GetCamera();
    XMMATRIX view    = camera->GetViewMatrix();
    XMMATRIX invView = XMMatrixInverse(NULL, view);
    invView.r[3].m128_f32[0] = 0.0f; // X方向の平行移動を消す
    invView.r[3].m128_f32[1] = 0.0f; // Y方向の平行移動を消す
    invView.r[3].m128_f32[2] = 0.0f; // Z方向の平行移動を消す

    // ---- 共通のGPUリソースをセット ----
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 深度テストを無効化（パーティクルが壁の後ろに隠れないようにする）
    Renderer::SetDepthEnable(false);

    // デフォルトテクスチャをセット
    // 将来: パーティクルごとに異なるテクスチャを使う場合は
    //       テクスチャ種別でソートしてから描画すると切り替え回数を減らせる
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_DefaultTexture);

    // ---- アクティブな全パーティクルを1つずつ描画 ----
    for (int i = 0; i < poolSize; i++)
    {
        const ParticleData& p = pool[i];
        if (!p.Active) continue; // 非アクティブはスキップ

        // ---- マテリアルに現在の色（アルファ含む）を反映 ----
        MATERIAL material{};
        material.Diffuse       = p.Color;   // パーティクルごとの色（フェードアウト中はアルファが下がる）
        material.TextureEnable = true;
        Renderer::SetMaterial(material);

        // ---- ワールド行列の組み立て ----
        // スケール × Z軸回転 × ビルボード回転 × 平行移動 の順に合成する。
        // この順番が大切で、スケールを先に掛けることで「サイズ変更 → カメラ向き → 位置移動」になる。
        XMMATRIX scale = XMMatrixScaling(p.Size, p.Size, p.Size);  // サイズ
        XMMATRIX rot   = XMMatrixRotationZ(p.Rotation);            // Z軸回転（スピン）
        XMMATRIX trans = XMMatrixTranslation(p.Position.x, p.Position.y, p.Position.z); // 位置
        XMMATRIX world = scale * rot * invView * trans;

        Renderer::SetWorldMatrix(world);
        Renderer::GetDeviceContext()->Draw(4, 0); // 頂点4つで四角形を1枚描く
    }

    // 深度テストを元に戻す（他のオブジェクトの描画に影響しないよう）
    Renderer::SetDepthEnable(true);
}
