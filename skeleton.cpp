#include "skeleton.h"

#include "assimp/scene.h"

using namespace DirectX;

// Assimpのaiマトリクス(aiMatrix4x4)は「列ベクトル規約」(v' = M*v、平行移動は最後の列 a4/b4/c4)で格納されている。
// このプロジェクトの既存コードはmul(頂点, 行列)という「行ベクトル規約」(平行移動は最後の行)を使っているため、
// 単純な要素コピーでは平行移動成分の位置がズレて破綻する。転置して変換する必要がある。
static XMMATRIX ToXMMatrix(const aiMatrix4x4& m)
{
	return XMMatrixSet(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4);
}


static void AddNodeRecursive(std::vector<Bone>& bones, const aiNode* node, int parentIndex)
{
	Bone bone;
	bone.Name           = node->mName.C_Str();
	bone.LocalBindMatrix = ToXMMatrix(node->mTransformation);
	bone.ParentIndex     = parentIndex;

	int thisIndex = (int)bones.size();
	bones.push_back(bone);

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		AddNodeRecursive(bones, node->mChildren[i], thisIndex);
}


void Skeleton::BuildFromScene(const aiScene* scene)
{
	Bones.clear();
	AddNodeRecursive(Bones, scene->mRootNode, -1);
}
