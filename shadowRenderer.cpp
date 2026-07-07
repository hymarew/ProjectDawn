#include "ShadowRenderer.h"
#include "Renderer.h"

// 実体（どこからでもアクセスできるようにするためのグローバルポインタ）
ShadowRenderer* g_ActiveShadowRenderer = nullptr;

void ShadowRenderer::Init()
{
    g_ActiveShadowRenderer = this;

    // ============================================
    // Shadow用VertexShaderの生成
    // ============================================
    // パス1（深度書き込み用）で使う専用の頂点シェーダーを読み込む。
    // 色の計算（ピクセルシェーダー）は不要なので、頂点シェーダーだけ作ればOK。
    Renderer::CreateVertexShader(&m_ShadowVS, &m_ShadowLayout, "shader\\ShadowDepthVS.cso");
    Renderer::CreatePixelShader(&m_ShadowPS,"shader\\ShadowDepthPS.cso");

    // ============================================
    // Light初期化
    // ============================================
    // 影の計算だけでなく、実際の明るさ計算にも使うライトの基本色
    m_Light.Diffuse = XMFLOAT4(1, 1, 1, 1);
    m_Light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1);

    // ============================================
    // LightView (ライトの位置と向き)
    // ============================================
    // カメラと同じように、ライトを「空間に浮かぶカメラ」として扱う。
    XMVECTOR lightPos = XMVectorSet(-30.0f, 30.0f, -30.0f, 1.0f); // ライトの位置（左手前の上空）
    XMVECTOR target = XMVectorZero();                           // ライトが向いている方向（原点）

    // ライトの位置からターゲットを見る行列を作成
    m_LightView = XMMatrixLookAtLH(lightPos, target, XMVectorSet(0, 1, 0, 0));

    // ============================================
    // LightProjection (ライトの照射範囲)
    // ============================================
    // 太陽光のような平行光源（ディレクショナルライト）を表現するため、
    // 遠近感のない「平行投影（Orthographic）」の行列を作成する。
    // 引数：(横幅, 縦幅, 手前のクリップ面, 奥のクリップ面)
    m_LightProj = XMMatrixOrthographicLH(50.0f, 50.0f, 0.1f, 100.0f);

    // ============================================
    // LightVP (View × Projection 行列)
    // ============================================
    // 毎フレーム計算するのは無駄なので、あらかじめ View行列 と Projection行列 を掛けておく。
    // シェーダー側では、これに各オブジェクトのWorld行列を掛けるだけで済む。
    m_LightVP = m_LightView * m_LightProj;
}

void ShadowRenderer::Uninit()
{
    // メモリリークを防ぐための解放処理
    if (m_ShadowLayout)
    {
        m_ShadowLayout->Release();
        m_ShadowLayout = nullptr;
    }

    if (m_ShadowVS)
    {
        m_ShadowVS->Release();
        m_ShadowVS = nullptr;
    }
}

void ShadowRenderer::Update()
{
    // 太陽の位置を時間経過で動かしたい場合などは、ここで m_LightView などを再計算する
}

void ShadowRenderer::Begin()
{
    // ============================================
    // ShadowPass開始 (描画先の切り替え)
    // ============================================
    // レンダーターゲットを NULL にし、DSV（深度バッファ）だけをセットする処理を呼ぶ
    Renderer::BeginShadowPass();

    // ============================================
    // ShadowShaderセット
    // ============================================
    // 頂点データのレイアウトを伝える
    Renderer::GetDeviceContext()->IASetInputLayout(m_ShadowLayout);

    // 影用の頂点シェーダー（ライト視点での座標変換のみを行う）をセット
    Renderer::GetDeviceContext()->VSSetShader(m_ShadowVS, nullptr, 0);

    // ★PixelShaderを NULL にする理由：
    // パス1では「深度（ライトからの距離）」だけをDSVに書き込めればよく、色を塗る必要がないため。
    // これにより、GPUの処理負荷を大幅に削減できる。
    //Renderer::GetDeviceContext()->PSSetShader(nullptr, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_ShadowPS,nullptr,0);

    // ============================================
    // LightMatrix設定 (定数バッファへの転送)
    // ============================================
    // シェーダーの定数バッファ (b6 など) に、ライトのVP行列を転送する
    Renderer::SetLightViewProjectionMatrix(m_LightVP);

    // （※この2行は現状のシェーダーでは使っていない可能性が高いが、将来用として保持）
    Renderer::SetViewMatrix(m_LightView);
    Renderer::SetProjectionMatrix(m_LightProj);
}

void ShadowRenderer::End()
{
    // 描画先を通常の画面（メインのレンダーターゲット）に戻す
    Renderer::EndShadowPass();
}

// ==========================================================
// 完成したシャドウマップを、シェーダーに渡すための関数
// ※ パス2（メインの描画）の直前で呼び出す
// ==========================================================
void ShadowRenderer::SetShadowMap()
{
    // Rendererクラスが管理しているシャドウマップ（読み込み用ビュー: SRV）を取得する
    ID3D11ShaderResourceView* shadowMap =Renderer::GetShadowMap();

    // 取得したシャドウマップを、ピクセルシェーダー(PS)にセット（バインド）する
    // 第1引数: セットするスロット番号（開始位置）。今回は t1 レジスタに送るので「1」
    // 第2引数: セットするテクスチャの数。今回はシャドウマップ1枚だけなので「1」
    // 第3引数: セットするテクスチャ（SRV）のポインタのアドレス
    Renderer::GetDeviceContext()->PSSetShaderResources(1, 1, &shadowMap);

    // 【補足】
    // これを呼ぶことで、HLSL(シェーダー)側の
    // Texture2D g_ShadowMap : register(t1);
    // に、パス1で描画された深度の画像がリンクされ、影の判定ができるようになる。
}