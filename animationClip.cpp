#include "animationClip.h"

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#pragma comment(lib, "assimp-vc143-mt.lib")

using namespace DirectX;


std::vector<AnimationClip> LoadAnimationClips(const char* FileName)
{
	std::vector<AnimationClip> clips;

	// Skeleton/FbxModelRendererと同じくDirectX(左手座標系)へ変換する。
	// ここではキーフレーム(アニメーションカーブ)しか読まないため、メッシュの有無に関わらずTriangulate等は不要。
	const aiScene* scene = aiImportFile(FileName, aiProcess_ConvertToLeftHanded);

	if (!scene || scene->mNumAnimations == 0)
	{
		if (scene) aiReleaseImport(scene);
		return clips;
	}

	for (unsigned int a = 0; a < scene->mNumAnimations; a++)
	{
		aiAnimation* anim = scene->mAnimations[a];

		// FBXにTicksPerSecondが記録されていない場合の一般的なフォールバック値
		double ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;

		AnimationClip clip;
		clip.Name = (anim->mName.length > 0) ? anim->mName.C_Str() : FileName;
		clip.Duration = (float)(anim->mDuration / ticksPerSecond);

		for (unsigned int c = 0; c < anim->mNumChannels; c++)
		{
			aiNodeAnim* channel = anim->mChannels[c];

			BoneAnimation boneAnim;
			boneAnim.BoneName = channel->mNodeName.C_Str();

			boneAnim.PositionKeys.reserve(channel->mNumPositionKeys);
			for (unsigned int k = 0; k < channel->mNumPositionKeys; k++)
			{
				const aiVectorKey& key = channel->mPositionKeys[k];
				Vec3Key vk;
				vk.Time = (float)(key.mTime / ticksPerSecond);
				vk.Value = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
				boneAnim.PositionKeys.push_back(vk);
			}

			boneAnim.RotationKeys.reserve(channel->mNumRotationKeys);
			for (unsigned int k = 0; k < channel->mNumRotationKeys; k++)
			{
				const aiQuatKey& key = channel->mRotationKeys[k];
				QuatKey qk;
				qk.Time = (float)(key.mTime / ticksPerSecond);
				// aiQuaternionは(w,x,y,z)の順で保持しているが、XMFLOAT4/XMVECTORの規約(x,y,z,w)に合わせる
				qk.Value = XMFLOAT4(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
				boneAnim.RotationKeys.push_back(qk);
			}

			boneAnim.ScaleKeys.reserve(channel->mNumScalingKeys);
			for (unsigned int k = 0; k < channel->mNumScalingKeys; k++)
			{
				const aiVectorKey& key = channel->mScalingKeys[k];
				Vec3Key vk;
				vk.Time = (float)(key.mTime / ticksPerSecond);
				vk.Value = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
				boneAnim.ScaleKeys.push_back(vk);
			}

			clip.BoneAnimations.push_back(std::move(boneAnim));
		}

		clips.push_back(std::move(clip));
	}

	aiReleaseImport(scene);
	return clips;
}
