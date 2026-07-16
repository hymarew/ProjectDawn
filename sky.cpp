#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "sky.h"
#include "keyboard.h"

#include "player.h"
#include "camera.h"

ID3D11VertexShader* SKY::m_VertexShader = nullptr;
ID3D11PixelShader* SKY::m_PixelShader = nullptr;
ID3D11InputLayout* SKY::m_VertexLayout = nullptr;

void SKY::Init()
{
    m_Layer = 1;
    m_Position = { 0.0f,10.0f,0.0f };

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\sky.obj");

    // 影用ではなくUnlit（ライティングなし）シェーダーを使うように変更する
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout, "shader\\unlitTextureVS.cso");
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\unlitTexturePS.cso");

    m_Scale = { 100.0f,100.0f,100.0f };
}

void SKY::Uninit()
{
    GameObject::Uninit();

    //m_VertexLayout->Release();
    //m_VertexShader->Release();
    //m_PixelShader->Release();

}

void SKY::Update(float dt)
{
    Camera* camera = Manager::GetCamera();
    m_Position = camera->GetPosition();

    GameObject::Update(dt);
}

void SKY::Draw()
{

    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

    // 追加: ImGuiの設定をライト構造体に反映させる
    Light.CastShadow = g_CastShadow;
    Renderer::SetLight(Light);    // ライト情報をシェーダーへ送る

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    //マトリクス設定
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    // 空はどれだけ遠くの地形・敵より奥にあっても常に背景として扱う。
    // デプス書き込みを無効化し、後から描く近景オブジェクトを絶対に隠さないようにする。
    Renderer::SetDepthEnable(false);
    GameObject::Draw(); //継承元のDraw()を呼び出す
    Renderer::SetDepthEnable(true);
}

void SKY::DrawShadow()
{
    // 空は影を落とさない（何も描かない）。
    // 以前は CheckInView 判定付きでシャドウマップへ描画していたが、SKY の位置は常に
    // カメラ位置と同一のため射影時に w≒0 となり、NaN 比較で判定結果が毎フレーム不安定だった。
    // 判定が true になったフレームだけ巨大な空の球体がシャドウマップ全体を覆い、
    // 「画面中の影がちかちかする」原因になっていた。
}