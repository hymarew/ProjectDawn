#pragma once
#include <vector>
#include "vector3.h"

class GameObject;
class Camera;

class Manager
{
private:
	static std::list<GameObject*> m_GameObject;
	static Camera* m_Camera;

public:
	static void Init();
	static void Uninit();
	static void Update(float dt);
	static void Draw();
	static void ImGuiDraw();
	static void DebugSystemInfo();

	static Camera* GetCamera() { return m_Camera; }	// カメラを取得する専用関数
	static void SetCamera(Camera* camera) { m_Camera = camera; }	// カメラを登録する専用関数

	//実行ファイルのサイズがでかくなりすぎたりするらしい
	template <typename T>	//テンプレート化(通常引数でクラスは送れないけど、リストとかテンプレートを組み合わせると行ける)
	static T* AddGameObject()
	{
		T* gameObject= new T;
		gameObject->Init();
		m_GameObject.push_back(gameObject);

		return gameObject;
	}

	// テンプレート関数
// 指定した型のゲームオブジェクトを取得する
	template <typename T>
	static T* GetGameObject()
	{
		// 登録されている全ゲームオブジェクトを走査
		for (GameObject* gameObject : m_GameObject)
		{
			// GameObject を指定した型 T に変換できるか確認
			// 変換できない場合は nullptr が返る
			T* find = dynamic_cast<T*>(gameObject);

			// 指定した型のオブジェクトが見つかったら返す
			if (find != nullptr)
				return find;
		}

		// 見つからなかった場合は nullptr を返す
		return nullptr;
	}

	// manager.h のクラス内に追加
	template <typename T>
	static std::vector<T*> GetGameObjects()
	{
		std::vector<T*> gameObjects;
		for (GameObject* gameObject : m_GameObject)
		{
			T* find = dynamic_cast<T*>(gameObject);
			if (find != nullptr)
				gameObjects.push_back(find);
		}
		return gameObjects;
	}

	// manager.h のクラス内に追加
	static std::list<GameObject*>& GetGameObjectList()
	{
		return m_GameObject;
	}

};

extern bool g_CastShadow;
extern bool g_ShowDebugUI;
extern bool g_ShowColliderDebug;

class BulletPool;
class BulletManager;
extern BulletPool    g_BulletPool;
extern BulletManager g_BulletManager;

class CollisionManager;
extern CollisionManager g_CollisionManager;

class ColliderDebugRenderer;
extern ColliderDebugRenderer g_ColliderDebugRenderer;

class ShadowRenderer;
extern ShadowRenderer* g_ShadowRenderer;

class SceneManager;
extern SceneManager g_SceneManager;

class StageManager;
extern StageManager g_StageManager;

class FadeManager;
extern FadeManager g_FadeManager;