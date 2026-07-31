#include "main.h"
#include "manager.h"
#include "inputManager.h"
#include "mouse.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "player.h"
#include "camera.h"
#include "weapon.h"
#include "gameContext.h"
#include "sphereCollider.h"
#include "collisionManager.h"
#include "GameConfig.h"
#include "playerLog.h"
#include "damageEffectManager.h"
void Player::Init()
{
    m_Layer = 1;
    m_Position = { 0.0f,0.0f,0.0f };

    AddComponent<ModelRenderer>(this)->Load("asset\\model\\Playerble.obj");

    //影を受けたり落とすために(ここら辺の細かい実験はまだ)
    // シェーダー読込
    Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
        "shader\\ShadowMapLightingVS.cso");

    Renderer::CreatePixelShader(&m_PixelShader,
        "shader\\ShadowMapLightingPS.cso");

    XMVECTOR dir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
    dir = XMVector3Normalize(dir);
    XMStoreFloat4(&Light.Direction, dir);   //光のベクトル
    Light.Position = XMFLOAT4(0.0f, 3.0f, 0.0f, 1.0f);    // ライトの位置
    Light.Diffuse = XMFLOAT4(1, 1, 1, 1);    // ライトの色
    Light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1);    // 環境光
    Light.PointLightParam = XMFLOAT4(300.0f, 0, 0, 0);    // ライトの届く距離
    // ライトの角度
    Light.SpotLightParam.x = cosf(XMConvertToRadians(25.0f)); // inner
    Light.SpotLightParam.y = cosf(XMConvertToRadians(35.0f)); // outer
    LightMoveSpeed = 0.1f;    // ライト移動速度

    // 武器選択画面で決めた装備（EquipLoadout）から Weapon 実体を生成する。
    // ステージ中の装備変更はできない（アクティブスロットの切替のみ可能）
    auto& ctx = GameContext::Instance();
    m_Equip.Init(&ctx.equipLoadout, &ctx.weaponFactory, &g_BulletPool);

    // コライダー登録
    auto* col = AddComponent<SphereCollider>(this);
    col->Setup(GameConfig::Collision::PLAYER_RADIUS, ColliderTag::Player);
    g_CollisionManager.Register(col);

    // HP 初期化
    m_Hp      = GameConfig::Player::MAX_HP;
    m_InvTimer = 0.0f;
}

void Player::Uninit()
{
    // コライダー解除（GameObject::Uninit() でコンポーネントが破棄される前に取得する）
    SphereCollider* col = GetComponent<SphereCollider>();
    if (col) g_CollisionManager.Unregister(col);

    GameObject::Uninit();

    m_VertexLayout->Release();
    m_VertexShader->Release();
    m_PixelShader->Release();

    // 装備中の武器実体を解放する
    m_Equip.Uninit();
}

void Player::Update(float dt)
{
    // ----------------------------------------------------
    // カメラ基準の移動方向ベクトル（前・右）を取得
    // ----------------------------------------------------
    Camera* camera = Manager::GetCamera();
    Vector3 forward = camera->GetForward();
    Vector3 right = camera->GetRight();

    // 移動が水平方向のみになるよう、Y成分（高さ）を消して正規化
    forward.y = 0.0f;
    forward.normalize();

    right.y = 0.0f;
    right.normalize();

    // ----------------------------------------------------
    // 各種パラメータの設定
    // ----------------------------------------------------
    // スロータイマーをカウントダウンする
    if (m_SlowTimer > 0.0f) m_SlowTimer -= dt;

    float gravity   = GameConfig::Physics::GRAVITY;
    float jumpForce = GameConfig::Physics::JUMP_FORCE;
    // スロー中は移動速度を減衰させる。抵抗は通常速度基準のままにして制動感を維持する。
    float speed  = GameConfig::Physics::PLAYER_SPEED * GetSpeedMult();
    float resist = GameConfig::Physics::PLAYER_SPEED / GameConfig::Physics::RESIST_DIVISOR;

    //ダッシュ
    if (InputManager::IsPressed(Action::SPRINT))
        speed *= GameConfig::Physics::SPRINT_MULT;

    // 移動（スティックの傾き量も自動で反映される）
    m_Velocity += forward * InputManager::GetMoveY() * speed * dt;
    m_Velocity += right * InputManager::GetMoveX() * speed * dt;

    // ジャンプ
    if (InputManager::IsTriggered(Action::JUMP))
    {
        m_Velocity.y += jumpForce;

        m_Scale.y = 1.5f;
        m_Scale.x = 0.5f;
        m_Scale.z = 0.5f;

    }

    m_Scale.x += (1.0f - m_Scale.x) * 0.1f;
    m_Scale.y += (1.0f - m_Scale.y) * 0.1f;
    m_Scale.z += (1.0f - m_Scale.z) * 0.1f;


    // ----------------------------------------------------
    // キャラクターの向き（回転）の更新
    // ----------------------------------------------------
    // カメラのYaw方向にプレイヤーを向かせる（EDFスタイル）
    m_Rotation.y = camera->GetRotation().y;

    // 重力の処理
    m_Velocity.y -= gravity * dt;  // 毎フレーム重力による落下処理を適用


    // ----------------------------------------------------
    // 抵抗力の適用と位置の最終更新
    // ----------------------------------------------------
    // 現在の速度に対して逆向きの力（抵抗）をかけ、徐々に減速させる
    m_Velocity += -m_Velocity * resist * dt;

    // 計算された最終的な速度を使って座標（位置）を更新
    m_Position += m_Velocity * dt;

    bool isOldGround = m_Ground;
    m_Ground = false;


    // ----------------------------------------------------
    // 地面との衝突判定（簡易版）
    // ----------------------------------------------------
    // Y座標が0未満（地面より下）になったら、地面の位置(Y=0)に押し戻して落下速度をリセット
    if (m_Position.y < 0.0f)
    {
        m_Position.y = 0.0f;
        m_Velocity.y = 0.0f;
        m_Ground = true;
    }

    if (!isOldGround && m_Ground)
    {
        m_Scale.y = 0.5f;
        m_Scale.x = 1.5f;
        m_Scale.z = 1.5f;
    }

    
    if (m_Ground)
    {
        m_MoveAnimation += m_Velocity.Length() * dt;
        m_Scale.y += sinf(m_MoveAnimation * 3.0f) * 0.02f;
    }



    // 無敵タイマーを減算する
    if (m_InvTimer > 0.0f)
        m_InvTimer -= dt;

    // 現在の武器を取得（未装備なら nullptr）
    Weapon* currentWeapon = GetWeapon();

    // 武器の時間管理（連射タイマー・リロードカウントダウン）を更新する。
    // 非アクティブ側のスロットもリロードが進むよう両方更新する
    for (int i = 0; i < (int)EquipSlot::Count; i++)
    {
        if (Weapon* w = m_Equip.GetWeapon((EquipSlot)i))
            w->Update(dt);
    }

    // リロード入力（R キー or ゲームパッド B）
    if (currentWeapon && InputManager::IsTriggered(Action::RELOAD))
        currentWeapon->StartReload();

    // ホイールで Primary ⇔ Secondary を切り替える
    if (Mouse::GetScrollDown() || Mouse::GetScrollUp())
    {
        m_Equip.SwitchSlot();
        currentWeapon = GetWeapon();  // 切り替え後の武器を取り直す
    }

    // 攻撃入力があれば現在の武器に発射を委譲する
    // singleShot 武器はトリガー入力（押した瞬間のみ）で発射する
    bool fireInput = currentWeapon &&
        (currentWeapon->IsSingleShot()
            ? InputManager::IsTriggered(Action::ATTACK)
            : InputManager::IsPressed(Action::ATTACK));
    if (fireInput)
    {
        Camera* cam = Manager::GetCamera();

        // クロスヘア（画面中央）が指すワールド座標を求める
        // カメラ位置からエイム方向へ十分遠い点を仮想のヒット地点とする
        Vector3 camPos   = cam->GetPosition();
        Vector3 aimDir   = cam->GetAimDirection();
        Vector3 aimPoint = camPos + aimDir * 1000.0f;

        // 銃口からヒット地点へ向かうベクトルを発射方向とする
        // → これによりクロスヘアと弾道が常に一致する
        Vector3 muzzlePos = m_Position + Vector3(0.0f, 1.5f, 0.0f);
        Vector3 shootDir  = aimPoint - muzzlePos;
        shootDir.normalize();

        currentWeapon->TryFire(muzzlePos, shootDir);
    }

    // 基底クラスのUpdate呼び出し（コンポーネントなどの更新処理）
    GameObject::Update(dt);
}

void Player::Draw()
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
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y + GameConfig::Player::MODEL_ROTATION_OFFSET, m_Rotation.z);
    trans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    world = scale * rot * trans;

    Renderer::SetWorldMatrix(world);

    GameObject::Draw(); //継承元のDraw()を呼び出す

}

void Player::DrawShadow()
{
    // ==================================================
    // 1. 自分の「位置・回転・大きさ」からワールド行列を計算
    // ==================================================
    XMMATRIX world, scale, rot, trans;
    scale = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);
    rot = XMMatrixRotationRollPitchYaw(m_Rotation.x, m_Rotation.y + GameConfig::Player::MODEL_ROTATION_OFFSET, m_Rotation.z);
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

// -------------------------------------------------------
// ApplySlow : スロー効果を付与する
//
// 既にスローがかかっている場合は、より長い duration の方を採用する。
// これにより蜘蛛が複数いても「一番長いスロー」が維持される。
// -------------------------------------------------------
void Player::ApplySlow(float duration, float mult)
{
    if (duration > m_SlowTimer)
    {
        m_SlowTimer = duration;
        m_SlowMult  = mult;
    }
}

// -------------------------------------------------------
// TakeDamage : ダメージを受ける
// -------------------------------------------------------
// 無敵時間中は無視する。HP が 0 以下になっても現状は放置（ゲームオーバー処理は別途実装）。
void Player::TakeDamage(float dmg, const char* source)
{
    if (m_InvTimer > 0.0f) return;
    m_Hp -= dmg;
    if (m_Hp < 0.0f) m_Hp = 0.0f;
    m_InvTimer = GameConfig::Player::INVINCIBLE_SEC;

    // 被弾UI演出（HUDシェイク・HP赤フラッシュ・赤ビネット）を再生する。
    // 画面全体の赤フラッシュやカメラシェイクはエイムを阻害するため使わない方針。
    g_DamageEffectManager.OnDamaged();

    g_PlayerLog.totalDamageTaken += dmg;
    g_PlayerLog.damageSources[source]++;

    if (m_Hp <= 0.0f)
    {
        g_PlayerLog.deathPosition = m_Position;
        g_PlayerLog.deathCause    = source;
    }
}