#pragma once

// Phase 1: FBXモデルの読み込み・静的表示(ボーン無し)
// Phase 2: メッシュにボーン/スキニング情報がある場合、VERTEX_3D_SKIN + Skeleton + ボーン行列バッファを構築し
//          GPUスキニングで表示する(バインドポーズ固定)。
// Phase 3: 別FBX(Idle.fbx等)からAnimationClip(キーフレーム列)を読み込めるようにした(animationClip.h)。
// Phase 4: PlayAnimation()で読み込んだクリップを毎フレーム再生し、キーフレーム補間→階層をたどったワールド変換→
//          ボーン行列を計算してGPUへ送る。Update()はComponent既存の仕組みからGameObject::Update経由で自動的に呼ばれる。
//
// MODEL/SUBSET/MODEL_MATERIALはModelRenderer(OBJ用)と同じ構造体を再利用し、
// 「単一頂点/インデックスバッファ + サブセット」という既存の描画方式に合わせる。
// スキニング専用の頂点フォーマット(VERTEX_3D_SKIN)・頂点シェーダー作成関数(Renderer::CreateVertexShaderSkinned)は
// 既存のVERTEX_3D/CreateVertexShaderとは別に追加したもので、他のモデル描画には一切影響しない。

#include "component.h"
#include "modelRenderer.h"
#include "skeleton.h"
#include "animationClip.h"
#include <string>
#include <unordered_map>


// スキニングモデル1つ分の共有データ(骨格のみ。同じFBXを複数インスタンスが使っても骨格定義自体は共通なのでプールする)
struct FbxSkinData
{
	static constexpr int MAX_BONES = 128; // shader\SkinningVS.hlsl の BoneMatrices[128] と一致させること

	Skeleton Skeleton;
};


class FbxModelRenderer : public Component
{
private:

	static std::unordered_map<std::string, MODEL*>       m_ModelPool;
	static std::unordered_map<std::string, FbxSkinData*> m_SkinPool;

	static void LoadModel(const char *FileName, MODEL *Model, FbxSkinData** OutSkinData);

	MODEL*       m_Model{};
	FbxSkinData* m_SkinData{}; // nullptrなら非スキニング(Phase1相当)

	// ここから下はインスタンス固有(同じモデルを複数のGameObjectが使っても再生状態は別々になる必要がある)
	ID3D11Buffer*  m_BoneMatrixBuffer{}; // Skinned時のみ生成。毎フレームUpdateSubresourceで更新する
	AnimationClip  m_CurrentClip;
	float          m_AnimTime = 0.0f;
	bool           m_HasAnimation = false;

public:

	static void UnloadAll();

	using Component::Component;

	void Uninit() override;
	void Update(float dt) override;
	void Load(const char *FileName);
	void Draw() override;
	void DrawShadow() override;

	// trueならボーン/スキニング情報を持つモデル。所有者(GameObject)はこれを見て
	// 通常のShadowMapLightingVS/PSではなくSkinningVS.hlsl+専用レイアウトを使う必要がある。
	bool IsSkinned() const { return m_SkinData != nullptr; }

	// 指定FBX(Idle.fbx等、メッシュ有無は問わない)からアニメーションクリップを読み込みループ再生を開始する。
	// IsSkinned()がfalseの場合は何もしない。
	void PlayAnimation(const char *FileName);
};
