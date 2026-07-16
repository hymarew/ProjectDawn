#pragma once
#include "transform.h"
#include "riftMaterial.h"
#include "riftRenderer.h"
#include "distortionPass.h"
#include "riftParticleEmitter.h"
#include "riftController.h"

// =====================================================
// SpaceRift : 「空間の裂け目」エフェクトを統括するファサード
//
//   SpaceRift
//   ├── Transform            … 位置・回転・スケール
//   ├── RiftRenderer          … 描画のみ
//   ├── RiftMaterial          … テクスチャ・色・Emission・UV速度などのパラメータ
//   ├── DistortionPass        … 画面歪み用の背景キャプチャ
//   ├── RiftParticleEmitter   … 中心から放つ青/白のエネルギー粒子
//   └── RiftController        … 時間経過・発光の脈動・ON/OFF管理
//
// 各パーツは単一責務のクラスに分割し、SpaceRiftはそれらを束ねて
// Init/Update/Draw の3メソッドだけを外部に公開する（Facadeパターン）。
//
// 【呼び出し順序】
//   Update(dt) はどのタイミングで呼んでもよい。
//   Draw() は BloomPass::BeginEmissivePass() 〜 EndEmissivePass() の間、
//   かつ Renderer::SetViewMatrix/SetProjectionMatrix が
//   現在のシーンカメラに設定された状態で呼ぶこと（内部でView/Projを再設定しない）。
// =====================================================
class SpaceRift
{
public:
    void Init(const Vector3& position);
    void Uninit();

    void Update(float dt);
    void Draw();

    void SetEnabled(bool enabled) { m_Controller.SetEnabled(enabled); }
    bool IsEnabled() const { return m_Controller.IsEnabled(); }

    Transform&          GetTransform() { return m_Transform; }
    const Transform&    GetTransform() const { return m_Transform; }
    RiftMaterial&        GetMaterial()  { return m_Material; }
    const RiftMaterial&  GetMaterial()  const { return m_Material; }

private:
    Transform           m_Transform;
    RiftMaterial        m_Material;
    RiftRenderer        m_Renderer;
    DistortionPass      m_Distortion;
    RiftParticleEmitter m_ParticleEmitter;
    RiftController      m_Controller;

    ID3D11ShaderResourceView* m_CrackMaskSRV = nullptr; // Crack_Mask.png（本体・地面投影で共有）
};
