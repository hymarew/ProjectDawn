#include "main.h"
#include "manager.h"
#include "inputManager.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "player.h"
#include "camera.h"

namespace
{
    constexpr float GRAVITY         = 20.0f;
    constexpr float JUMP_FORCE      = 8.0f;
    constexpr float PLAYER_SPEED    = 8.0f;
    constexpr float RESIST_DIVISOR  = 0.2f;
}

void Player::Init()
{
    m_Layer    = 1;
    m_Position = { 0.0f, 0.0f, 0.0f };

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\player.obj");

    // 影を受けたり落とすためのシェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\ShadowMapLightingVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\ShadowMapLightingPS.cso");

    XMVECTOR dir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
    dir = XMVector3Normalize(dir);
    XMStoreFloat4(&m_Light.Direction, dir);   // 光のベクトル
    m_Light.Position       = XMFLOAT4(0.0f, 3.0f, 0.0f, 1.0f);
    m_Light.Diffuse        = XMFLOAT4(1, 1, 1, 1);
    m_Light.Ambient        = XMFLOAT4(0.1f, 0.1f, 0.1f, 1);
    m_Light.PointLightParam = XMFLOAT4(300.0f, 0, 0, 0);
    m_Light.SpotLightParam.x = cosf(XMConvertToRadians(25.0f)); // inner
    m_Light.SpotLightParam.y = cosf(XMConvertToRadians(35.0f)); // outer
    m_Light.Enable     = true;
    m_Light.CastShadow = true;
}

void Player::Uninit()
{
    GameObject::Uninit();

    m_VertexLayout->Release();
    m_VertexShader->Release();
    m_PixelShader->Release();
}

void Player::Update(float dt)
{
    // カメラ基準の移動方向ベクトル（前・右）を取得
    Camera* camera   = Manager::GetGameObject<Camera>();
    Vector3 forward  = camera ? camera->GetForward() : Vector3(0.0f, 0.0f, 1.0f);
    Vector3 right    = camera ? camera->GetRight()   : Vector3(1.0f, 0.0f, 0.0f);

    // 移動が水平方向のみになるよう、Y成分（高さ）を消して正規化
    forward.y = 0.0f;
    forward.normalize();

    right.y = 0.0f;
    right.normalize();

    float speed  = PLAYER_SPEED;
    float resist = PLAYER_SPEED / RESIST_DIVISOR;

    // 移動（スティックの傾き量も自動で反映される）
    m_Velocity += forward * InputManager::GetMoveY() * speed * dt;
    m_Velocity += right   * InputManager::GetMoveX() * speed * dt;

    // ジャンプ
    if (m_Ground && InputManager::IsTriggered(Action::JUMP))
    {
        m_Velocity.y += JUMP_FORCE;
    }

    // カメラのYaw方向にプレイヤーを向かせる
    if (camera) m_Rotation.y = camera->GetRotation().y;

    // 重力
    m_Velocity.y -= GRAVITY * dt;

    // 抵抗力を掛けて減速させる
    m_Velocity += -m_Velocity * resist * dt;

    // 位置更新
    m_Position += m_Velocity * dt;

    // 地面との衝突判定（簡易版）
    m_Ground = false;
    if (m_Position.y < 0.0f)
    {
        m_Position.y = 0.0f;
        m_Velocity.y = 0.0f;
        m_Ground = true;
    }

    if (m_Ground)
    {
        m_MoveAnimation += m_Velocity.Length() * dt;
    }

    // 基底クラスのUpdate呼び出し（コンポーネントなどの更新処理）
    GameObject::Update(dt);
}

void Player::Draw()
{
    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Renderer::SetLight(m_Light);    // ライト情報をシェーダーへ送る

    // シェーダ設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

    // マトリクス設定
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    GameObject::Draw(); // 継承元のDraw()を呼び出す（コンポーネントのモデル描画）
}

void Player::DrawShadow()
{
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot   = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    GameObject::DrawShadow();
}
