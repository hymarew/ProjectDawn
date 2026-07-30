#pragma once

// FBXから読み込んだ1本のアニメーション(Idle/Run/Jumpなど)を保持する。
// メッシュ+スキンを読み込むFbxModelRendererとは独立した関数として提供する
// (キャラクターによっては、メッシュ+スキン+アニメーションを1本のFBXにまとめて含む場合と、
//  アニメーションだけを個別のFBXとして持つ場合の両方があるため)。

#include <DirectXMath.h>
#include <string>
#include <vector>

struct Vec3Key
{
	float              Time = 0.0f; // 秒
	DirectX::XMFLOAT3  Value{};
};

struct QuatKey
{
	float              Time = 0.0f; // 秒
	DirectX::XMFLOAT4  Value{}; // x, y, z, w
};

// 1本のボーン(ノード名で対応するSkeleton::Boneと紐付ける)に対するキーフレーム列
struct BoneAnimation
{
	std::string            BoneName;
	std::vector<Vec3Key>   PositionKeys;
	std::vector<QuatKey>   RotationKeys;
	std::vector<Vec3Key>   ScaleKeys;
};

// 1つのアニメーションクリップ(例: "Idle", "Run")
struct AnimationClip
{
	std::string                 Name;
	float                       Duration = 0.0f; // 秒

	std::vector<BoneAnimation>  BoneAnimations;

	const BoneAnimation* FindBoneAnimation(const std::string& boneName) const
	{
		for (const BoneAnimation& b : BoneAnimations)
			if (b.BoneName == boneName)
				return &b;
		return nullptr;
	}
};

// FBXファイルに含まれる全アニメーションクリップを読み込む(Mixamoの1ファイル1クリップ想定だが、
// 複数含まれる場合もそれぞれ識別できるようvectorで返す)
std::vector<AnimationClip> LoadAnimationClips(const char* FileName);
