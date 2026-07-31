#pragma once

// Phase2: FBXから読み込んだボーン階層を保持する最小限のSkeleton。
// このPhaseではバインドポーズ(ノードの初期ローカル行列)のみを使い、
// 再生中のアニメーションは持たない(Phase3/4で別途扱う)。

#include <DirectXMath.h>
#include <string>
#include <vector>

struct aiScene;

struct Bone
{
	std::string       Name;
	DirectX::XMMATRIX LocalBindMatrix = DirectX::XMMatrixIdentity(); // バインドポーズ時のノードローカル変換
	DirectX::XMMATRIX OffsetMatrix    = DirectX::XMMatrixIdentity(); // メッシュ空間→ボーン空間(スキニング行列の算出に使う)
	int               ParentIndex     = -1;                         // 親のBoneIndex(このSkeleton内でのインデックス)。ルートは-1
	bool              HasOffset       = false;                      // 実際にスキニングへ使われるボーンか(falseなら中間ノードなど)
	int               GpuIndex        = -1;                         // HasOffsetがtrueの場合、GPUへ送るボーン行列配列でのインデックス
};

struct Skeleton
{
	std::vector<Bone> Bones;
	int                GpuBoneCount = 0; // HasOffset==trueなボーンの数(GPUバッファへ送る行列数)

	int FindBoneIndex(const std::string& name) const
	{
		for (size_t i = 0; i < Bones.size(); i++)
			if (Bones[i].Name == name)
				return (int)i;
		return -1;
	}

	// シーン全体のノード階層をそのままSkeletonへ取り込む。
	// (どのノードが実際にスキンウェイトを持つボーンかは、後からOffsetMatrixを設定して確定する)
	void BuildFromScene(const aiScene* scene);
};
