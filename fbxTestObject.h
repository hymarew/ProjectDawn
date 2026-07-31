#pragma once

// Phase 1/2 動作確認用の独立したテストオブジェクト。
// Playerには組み込まず、FbxTestScene からのみ生成する想定。
// asset\model\fbx\Idle.fbx をFbxModelRendererで読み込み、静止表示する。
// FbxModelRenderer::IsSkinned() に応じて、スキニング無し用/有り用のシェーダーを切り替える。

#include <d3d11.h>
#include <vector>
#include "gameObject.h"
#include "renderer.h"

class FbxModelRenderer;

class FbxTestObject : public GameObject
{
private:
	static ID3D11InputLayout*  m_VertexLayout;
	static ID3D11VertexShader* m_VertexShader;
	static ID3D11PixelShader*  m_PixelShader;

	// スキニングモデル(ボーン有りFBX)用。IsSkinned()がtrueのときはこちらをバインドする
	static ID3D11InputLayout*  m_SkinVertexLayout;
	static ID3D11VertexShader* m_SkinVertexShader;

	static bool m_IsLoaded;

	FbxModelRenderer* m_ModelRenderer = nullptr;

	LIGHT Light;

public:
	void Init()           override;
	void Update(float dt) override;
	void Draw()           override;
	const char* GetName() override { return "FbxTestObject"; }

	bool IsSkinned() const;

	// 指定FBXのアニメーションクリップに切り替える(Idle/Run/Jump切り替え用)
	void PlayAnimation(const char* fileName);

	// デバッグ表示用: 全ボーンのワールド座標(このオブジェクトのScale/Rotation/Positionを適用済み)と、
	// その親ボーンのインデックス(ルートは-1)、実際にスキニングへ使われるボーンかどうかを取得する。
	// IsSkinned()がfalseならfalseを返す。
	bool GetBoneWorldPositions(std::vector<Vector3>& outPositions, std::vector<int>& outParentIndices, std::vector<bool>& outHasOffset) const;

	// Phase4: アニメーション再生状態のデバッグ表示用
	bool        HasAnimation() const;
	const char* GetCurrentClipName() const;
	float       GetAnimTime() const;
	float       GetClipDuration() const;
	int         GetSkeletonBoneCount() const;
	int         GetMatchedAnimationBoneCount() const;
};
