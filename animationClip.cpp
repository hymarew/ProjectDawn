#include "main.h"
#include "animationClip.h"

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#pragma comment(lib, "assimp-vc143-mt.lib")

using namespace DirectX;


std::vector<AnimationClip> LoadAnimationClips(const char* FileName)
{
	std::vector<AnimationClip> clips;

	// メッシュ・ボーン階層と同じ変換規約(Assimp右手系→DirectX左手系)を合わせるため、
	// FbxModelRenderer::LoadModelと同じポストプロセスフラグでインポートする。
	const aiScene* scene = aiImportFile(FileName,
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_GenNormals);

	if (!scene || scene->mNumAnimations == 0)
	{
		if (scene) aiReleaseImport(scene);
		return clips;
	}

	for (unsigned int a = 0; a < scene->mNumAnimations; a++)
	{
		aiAnimation* anim = scene->mAnimations[a];

		// mTicksPerSecondが0の場合(FBXでは稀に未設定)、Assimpの慣例に合わせ25fpsとみなす
		double ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;

		AnimationClip clip;
		clip.Name     = anim->mName.C_Str();
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
				Vec3Key posKey;
				posKey.Time  = (float)(key.mTime / ticksPerSecond);
				posKey.Value = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
				boneAnim.PositionKeys.push_back(posKey);
			}

			boneAnim.RotationKeys.reserve(channel->mNumRotationKeys);
			for (unsigned int k = 0; k < channel->mNumRotationKeys; k++)
			{
				const aiQuatKey& key = channel->mRotationKeys[k];
				QuatKey rotKey;
				rotKey.Time  = (float)(key.mTime / ticksPerSecond);
				rotKey.Value = XMFLOAT4(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
				boneAnim.RotationKeys.push_back(rotKey);
			}

			boneAnim.ScaleKeys.reserve(channel->mNumScalingKeys);
			for (unsigned int k = 0; k < channel->mNumScalingKeys; k++)
			{
				const aiVectorKey& key = channel->mScalingKeys[k];
				Vec3Key scaleKey;
				scaleKey.Time  = (float)(key.mTime / ticksPerSecond);
				scaleKey.Value = XMFLOAT3(key.mValue.x, key.mValue.y, key.mValue.z);
				boneAnim.ScaleKeys.push_back(scaleKey);
			}

			clip.BoneAnimations.push_back(boneAnim);
		}

		clips.push_back(clip);
	}

	aiReleaseImport(scene);
	return clips;
}
