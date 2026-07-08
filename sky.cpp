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
    Camera* camera = Manager::GetCamera();
    if (!camera->CheckInView(m_Position, 3.0f))  //メインカメラで判定すると、影が不自然になるらしいから、マージン追加
        return;
    // ==================================================
    // 1. 自分の「位置・回転・大きさ」からワールド行列を計算
    // ==================================================
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    // ==================================================
    // 2. GPUに「今から描くモデルはここ(world)に配置してね」と伝える
    // ==================================================
    Renderer::SetWorldMatrix(world);

    // ==================================================
    // 3. 実際のモデル(頂点)の描画は、コンポーネント(ModelRenderer)に委譲する
    // ==================================================
    // 基底クラスの DrawShadow() を呼ぶと、自分が持っている
    // 全てのコンポーネントの DrawShadow() が順番に呼ばれる。
    GameObject::DrawShadow();
}