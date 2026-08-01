#include "main.h"
#include "skeleton.h"

#include <fbxsdk.h>

using namespace DirectX;


// FbxAMatrix(列ベクトル規約、平行移動が最終列)からDirectXの行ベクトル規約へ転置して変換する。
// FbxAMatrixをT/R/Sへ分解してからDirectXの行ベクトル規約(Scale*Rotation*Translation)で組み直す。
// GetColumn()で要素を直接読んで転置する方式は、平行移動成分が常に0になる不具合が確認できたため
// (原因未特定だが、恐らくFbxAMatrixの内部規約の思い込み違い)、ドキュメント化されたGetT/GetQ/GetSに統一する。
static XMMATRIX ToXMMatrix(const FbxAMatrix& m)
{
	FbxVector4    t = m.GetT();
	FbxQuaternion q = m.GetQ();
	FbxVector4    s = m.GetS();

	XMMATRIX sm = XMMatrixScaling((float)s[0], (float)s[1], (float)s[2]);
	XMMATRIX rm = XMMatrixRotationQuaternion(XMVectorSet((float)q[0], (float)q[1], (float)q[2], (float)q[3]));
	XMMATRIX tm = XMMatrixTranslation((float)t[0], (float)t[1], (float)t[2]);

	return sm * rm * tm;
}


static void AddNodeRecursive(Skeleton& skeleton, FbxNode* node, int parentIndex)
{
	Bone bone;
	bone.Name            = node->GetName();
	bone.LocalBindMatrix = ToXMMatrix(node->EvaluateLocalTransform());
	bone.ParentIndex      = parentIndex;

	skeleton.Bones.push_back(bone);
	int thisIndex = (int)skeleton.Bones.size() - 1;

	// 親を必ず子より前のインデックスに置く(この関数の呼び出し順そのままでOK)ため、
	// 子の再帰呼び出しはここで(pushの後で)行う。
	for (int i = 0; i < node->GetChildCount(); i++)
		AddNodeRecursive(skeleton, node->GetChild(i), thisIndex);
}


void Skeleton::BuildFromScene(FbxScene* scene)
{
	Bones.clear();
	AddNodeRecursive(*this, scene->GetRootNode(), -1);
}
