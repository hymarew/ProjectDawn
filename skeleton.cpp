#include "main.h"
#include "skeleton.h"

#include "assimp/scene.h"

using namespace DirectX;


// Assimpの列ベクトル規約(平行移動が最後の列)からDirectXの行ベクトル規約へ転置して変換する。
static XMMATRIX ToXMMatrix(const aiMatrix4x4& m)
{
	return XMMatrixSet(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4);
}


static void AddNodeRecursive(Skeleton& skeleton, const aiNode* node, int parentIndex)
{
	Bone bone;
	bone.Name           = node->mName.C_Str();
	bone.LocalBindMatrix = ToXMMatrix(node->mTransformation);
	bone.ParentIndex     = parentIndex;

	skeleton.Bones.push_back(bone);
	int thisIndex = (int)skeleton.Bones.size() - 1;

	// 親を必ず子より前のインデックスに置く(この関数の呼び出し順そのままでOK)ため、
	// 子の再帰呼び出しはここで(pushの後で)行う。
	for (unsigned int i = 0; i < node->mNumChildren; i++)
		AddNodeRecursive(skeleton, node->mChildren[i], thisIndex);
}


void Skeleton::BuildFromScene(const aiScene* scene)
{
	Bones.clear();
	AddNodeRecursive(*this, scene->mRootNode, -1);
}
