#pragma once

// Phase 1: FBXモデルの読み込み・静的表示(ボーン無し)
// Phase 2: メッシュにボーン/スキニング情報がある場合、VERTEX_3D_SKIN + Skeleton + ボーン行列バッファを構築し
//          GPUスキニングで表示する。
// Phase 3: 別途animationClip.hのLoadAnimationClips()でアニメーションクリップ(キーフレーム列)を読み込める。
// Phase 4: PlayAnimation()で読み込んだクリップを毎フレーム再生する。Update()はComponentの仕組みから
//          GameObject::Update経由で自動的に呼ばれる。
//
// MODEL/SUBSET/MODEL_MATERIALはModelRenderer(OBJ用)と同じ構造体を再利用し、
// 「単一頂点/インデックスバッファ + サブセット」という既存の描画方式に合わせる。
// スキニング専用の頂点フォーマット(VERTEX_3D_SKIN)・頂点シェーダー作成関数(Renderer::CreateVertexShaderSkinned)は
// 既存のVERTEX_3D/CreateVertexShaderとは別に追加したもので、他のモデル描画には一切影響しない。
//
// メッシュ・ボーン・アニメーションの読み込みはAssimpではなく Autodesk FBX SDK を使用する。

#include "component.h"
#include "modelRenderer.h"
#include "skeleton.h"
#include "animationClip.h"
#include <string>
#include <unordered_map>
#include <vector>


// スキニングモデル1つ分の共有データ(骨格のみ。同じFBXを複数インスタンスが使っても骨格定義自体は共通なのでプールする)
struct FbxSkinData
{
	static constexpr int MAX_BONES = 128; // shader\SkinningVS.hlsl の MAX_BONES と一致させること

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
	ID3D11Buffer*  m_BoneMatrixBuffer{};
	AnimationClip  m_CurrentClip;
	float          m_AnimTime = 0.0f;
	bool           m_HasAnimation = false;

	// デバッグ表示用: 直近のUpdate/Loadで計算した各ボーンのモデル空間ワールド変換(バインドポーズ or 再生中の姿勢)
	std::vector<DirectX::XMMATRIX> m_LastGlobalTransforms;

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

	bool        HasAnimation() const { return m_HasAnimation; }
	const char* GetCurrentClipName() const { return m_CurrentClip.Name.c_str(); }
	float       GetAnimTime() const { return m_AnimTime; }
	float       GetClipDuration() const { return m_CurrentClip.Duration; }

	// デバッグ用: 骨格の総ボーン数のうち、再生中クリップの名前一致で実際にアニメーションが適用されているボーン数
	int GetSkeletonBoneCount() const { return m_SkinData ? (int)m_SkinData->Skeleton.Bones.size() : 0; }
	int GetMatchedAnimationBoneCount() const;

	// デバッグ表示用: 直近フレームで計算された全ボーンのモデル空間座標と、その親ボーンのインデックス(ルートは-1)、
	// 実際にスキニングへ使われるボーンかどうか(false = 中間ノード)を取得する。
	// アニメーション再生中はその姿勢を反映する。IsSkinned()がfalseの場合は何もせずfalseを返す。
	bool GetCurrentBonePositions(std::vector<DirectX::XMFLOAT3>& outPositions, std::vector<int>& outParentIndices, std::vector<bool>& outHasOffset) const;
};
