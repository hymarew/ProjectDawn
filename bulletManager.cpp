#include "bulletManager.h"
#include "bulletPool.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "camera.h"
#include "manager.h"

// bullet.obj のパスを定数として定義（複数箇所で使うため）
static const char* BULLET_MODEL_PATH = "asset\\model\\bullet.obj";

// -------------------------------------------------------
// Init : GPU リソースの初期化
// -------------------------------------------------------
// bullet.obj を ModelRenderer の静的キャッシュに登録する。
// シェーダーは通常の3Dオブジェクトと同じものを使い回す。
void BulletManager::Init()
{
    // モデルを静的キャッシュに読み込む（以降は GetCachedModel() で取得できる）
    ModelRenderer::Preload(BULLET_MODEL_PATH);

    // 頂点シェーダーと入力レイアウトをセットで生成する
    Renderer::CreateVertexShader(&m_VertexShader, &m_InputLayout,
        "shader\\ShadowMapLightingVS.cso");

    // ピクセルシェーダーを生成する
    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\ShadowMapLightingPS.cso");
}

// -------------------------------------------------------
// Uninit : GPU リソースの解放
// -------------------------------------------------------
void BulletManager::Uninit()
{
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_PixelShader)  { m_PixelShader->Release();  m_PixelShader  = nullptr; }
    if (m_InputLayout)  { m_InputLayout->Release();  m_InputLayout  = nullptr; }
}

// -------------------------------------------------------
// Draw : アクティブな弾を1体ずつ描画する
// -------------------------------------------------------
// 弾の数は最大でも1000程度なので、インスタンシングを使わず
// 1体ずつワールド行列を設定して DrawIndexed を呼ぶ方式を採用。
void BulletManager::Draw(BulletPool& pool)
{
    // キャッシュからモデルデータ（頂点バッファ・インデックスバッファ）を取得
    MODEL* model = ModelRenderer::GetCachedModel(BULLET_MODEL_PATH);
    if (!model) return; // モデルが読み込まれていなければ何もしない

    // シェーダーと入力レイアウトをGPUにセット
    Renderer::GetDeviceContext()->IASetInputLayout(m_InputLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

    Camera* camera = Manager::GetCamera();

    for (auto& bullet : pool.GetPool())
    {
        // 非アクティブ（未使用）スロットはスキップ
        if (!bullet.isActive) continue;

        // カメラの視野外にある弾は描画コストをかけずにスキップ（視錐台カリング）
        if (!camera->CheckInView(bullet.position)) continue;

        // 弾の位置からワールド行列（平行移動のみ）を生成してGPUに送る
        // 弾にはスケールと回転は不要なので XMMatrixTranslation だけでOK
        XMMATRIX world = XMMatrixTranslation(
            bullet.position.x, bullet.position.y, bullet.position.z);
        Renderer::SetWorldMatrix(world);

        // モデルのサブセット（マテリアル単位）ごとに描画する
        UINT stride = sizeof(VERTEX_3D);
        UINT offset = 0;
        for (unsigned int i = 0; i < model->SubsetNum; i++)
        {
            // マテリアル情報（色・光沢など）をシェーダーに送る
            Renderer::SetMaterial(model->SubsetArray[i].Material.Material);

            // テクスチャが存在すれば、スロット0にバインドする
            if (model->SubsetArray[i].Material.Texture)
                Renderer::GetDeviceContext()->PSSetShaderResources(
                    0, 1, &model->SubsetArray[i].Material.Texture);

            // 頂点バッファとインデックスバッファをセットして描画
            Renderer::GetDeviceContext()->IASetVertexBuffers(
                0, 1, &model->VertexBuffer, &stride, &offset);
            Renderer::GetDeviceContext()->IASetIndexBuffer(
                model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Renderer::GetDeviceContext()->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Renderer::GetDeviceContext()->DrawIndexed(
                model->SubsetArray[i].IndexNum,
                model->SubsetArray[i].StartIndex, 0);
        }
    }
}

// -------------------------------------------------------
// DrawShadow : 影パス用の描画
// -------------------------------------------------------
// 影パスではシェーダーを変える必要がないため（ShadowRenderer 側で設定済み）、
// ワールド行列をセットして頂点を送るだけでよい。
void BulletManager::DrawShadow(BulletPool& pool)
{
    MODEL* model = ModelRenderer::GetCachedModel(BULLET_MODEL_PATH);
    if (!model) return;

    Camera* camera = Manager::GetCamera();

    for (auto& bullet : pool.GetPool())
    {
        if (!bullet.isActive) continue;

        // 影は少し広めの判定で行う（影の端が急に切れないようにするため）
        if (!camera->CheckInView(bullet.position, 3.0f)) continue;

        XMMATRIX world = XMMatrixTranslation(
            bullet.position.x, bullet.position.y, bullet.position.z);
        Renderer::SetWorldMatrix(world);

        UINT stride = sizeof(VERTEX_3D);
        UINT offset = 0;
        for (unsigned int i = 0; i < model->SubsetNum; i++)
        {
            Renderer::GetDeviceContext()->IASetVertexBuffers(
                0, 1, &model->VertexBuffer, &stride, &offset);
            Renderer::GetDeviceContext()->IASetIndexBuffer(
                model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            Renderer::GetDeviceContext()->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Renderer::GetDeviceContext()->DrawIndexed(
                model->SubsetArray[i].IndexNum,
                model->SubsetArray[i].StartIndex, 0);
        }
    }
}
