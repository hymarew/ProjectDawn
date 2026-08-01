#include "main.h"
#include "animationClip.h"

#include <fbxsdk.h>
#include <cmath>

#pragma comment(lib, "libfbxsdk-mt.lib")
#pragma comment(lib, "libxml2-mt.lib")
#pragma comment(lib, "zlib-mt.lib")

using namespace DirectX;


static void CollectNodesRecursive(FbxNode* node, std::vector<FbxNode*>& outNodes)
{
	outNodes.push_back(node);
	for (int i = 0; i < node->GetChildCount(); i++)
		CollectNodesRecursive(node->GetChild(i), outNodes);
}


std::vector<AnimationClip> LoadAnimationClips(const char* FileName)
{
	std::vector<AnimationClip> clips;

	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ioSettings = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ioSettings);

	FbxImporter* importer = FbxImporter::Create(manager, "");
	if (!importer->Initialize(FileName, -1, manager->GetIOSettings()))
	{
		importer->Destroy();
		manager->Destroy();
		return clips;
	}

	FbxScene* scene = FbxScene::Create(manager, "AnimScene");
	importer->Import(scene);
	importer->Destroy();

	// メッシュ側(FbxModelRenderer::LoadModel)と同じ座標系・単位系に揃える。
	// (揃えないと、ボーン名は一致してもローカル変換の基準がずれてしまう)
	// ConvertScene()はハンドネス変更を正しく表現できないため、DeepConvertScene()を使う
	// (fbxModelRenderer.cpp::LoadModelと同じ理由)。
	FbxAxisSystem::DirectX.DeepConvertScene(scene);
	FbxSystemUnit::m.ConvertScene(scene);

	std::vector<FbxNode*> nodes;
	CollectNodesRecursive(scene->GetRootNode(), nodes);

	FbxAnimEvaluator* evaluator = scene->GetAnimationEvaluator();

	const int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
	for (int a = 0; a < stackCount; a++)
	{
		FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(a);
		scene->SetCurrentAnimationStack(stack);

		FbxTimeSpan span = stack->GetLocalTimeSpan();
		double startSec = span.GetStart().GetSecondDouble();
		double stopSec  = span.GetStop().GetSecondDouble();
		double duration = stopSec - startSec;
		if (duration <= 0.0) continue;

		// サンプリング周波数(Hz)。生カーブを解析せず、一定間隔でローカル変換を焼き込む方式にすることで、
		// FBXのピボット分解(PreRotation等)の解決をFbxAnimEvaluatorに任せられる。
		const double sampleRate = 30.0;
		const int sampleCount = (int)std::ceil(duration * sampleRate) + 1;

		AnimationClip clip;
		clip.Name     = stack->GetName();
		clip.Duration = (float)duration;
		clip.BoneAnimations.resize(nodes.size());
		for (size_t n = 0; n < nodes.size(); n++)
			clip.BoneAnimations[n].BoneName = nodes[n]->GetName();

		for (int s = 0; s < sampleCount; s++)
		{
			double t = startSec + (double)s / sampleRate;
			if (t > stopSec) t = stopSec;
			float localTime = (float)(t - startSec);

			FbxTime fbxTime;
			fbxTime.SetSecondDouble(t);

			for (size_t n = 0; n < nodes.size(); n++)
			{
				FbxAMatrix local = evaluator->GetNodeLocalTransform(nodes[n], fbxTime);

				FbxVector4    pos   = local.GetT();
				FbxQuaternion rot   = local.GetQ();
				FbxVector4    scale = local.GetS();

				Vec3Key posKey;   posKey.Time   = localTime; posKey.Value = XMFLOAT3((float)pos[0], (float)pos[1], (float)pos[2]);
				QuatKey rotKey;   rotKey.Time   = localTime; rotKey.Value = XMFLOAT4((float)rot[0], (float)rot[1], (float)rot[2], (float)rot[3]);
				Vec3Key scaleKey; scaleKey.Time = localTime; scaleKey.Value = XMFLOAT3((float)scale[0], (float)scale[1], (float)scale[2]);

				clip.BoneAnimations[n].PositionKeys.push_back(posKey);
				clip.BoneAnimations[n].RotationKeys.push_back(rotKey);
				clip.BoneAnimations[n].ScaleKeys.push_back(scaleKey);
			}
		}

		clips.push_back(std::move(clip));
	}

	scene->Destroy();
	manager->Destroy();

	return clips;
}
