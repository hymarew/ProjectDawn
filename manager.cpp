//システム系
#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "shadowRenderer.h"
#include "camera.h"
#include "inputManager.h"
#include "debugInfo.h"
#include "inputVisualizer.h"
#include "input.h"
#include "sceneManager.h"
#include "audio.h"

//オブジェクト系
#include "player.h"
#include "enemyPool.h"
#include "enemySpawner.h"
#include "bulletPool.h"
#include "bulletManager.h"
#include "weapon.h"
#include "collisionManager.h"
#include "damageVisualizer.h"

#include "GameConfig.h"
#include "colliderDebugRenderer.h"
#include "fadeManager.h"
#include "saveManager.h"
#include "particleManager.h"

//インスタンス
std::list<GameObject*> Manager::m_GameObject;
ShadowRenderer* g_ShadowRenderer = nullptr;

BulletPool       g_BulletPool;
BulletManager    g_BulletManager;
CollisionManager g_CollisionManager;
bool g_CastShadow        = true;
bool g_ShowDebugUI       = false;
bool g_ShowColliderDebug = false;
Camera* Manager::m_Camera = nullptr;


void Manager::Init()
{
	Renderer::Init();
	g_ShadowRenderer = new ShadowRenderer();
	g_ShadowRenderer->Init();
	InputManager::Init();
	Audio::InitMaster();

	// セーブデータをロードする。ファイルがなければデフォルト値で新規生成して保存する。
	// SceneManager::Init より前に呼ぶことで、初回シーン（TitleScene）表示前にデータが揃う。
	g_SaveManager.Load();

	// シーン管理を初期化（TitleScene からスタート）
	g_SceneManager.Init();
}


void Manager::Uninit()
{
	g_SceneManager.Uninit();

	g_ShadowRenderer->Uninit();
	delete g_ShadowRenderer;
	Audio::UninitMaster();

	Renderer::Uninit();
}

void Manager::Update(float dt)
{
	InputManager::Update();

	// 現在のシーンを更新
	g_SceneManager.Update(dt);
}

void Manager::Draw()
{
	// 現在のシーンが描画を担当する
	g_SceneManager.Draw();
}

void Manager::ImGuiDraw()
{
	if (g_ShowDebugUI)
	{
	ImGui::Begin("Debug");

	//--------------------------------
	// 状態表示
	//--------------------------------
	if (ImGui::CollapsingHeader("DebugInfo"))
	{
		DebugSystemInfo();
		ImGui::SeparatorText("Shadow Settings");
		ImGui::Checkbox("Cast Shadow", &g_CastShadow);
		ImGui::SeparatorText("Collider Debug");
		ImGui::Checkbox("Show Colliders (wireframe)", &g_ShowColliderDebug);
	}

	// ===== Manager管理下の全GameObjectを表示 =====
	if (ImGui::CollapsingHeader("GameObjects"))
	{
		int index = 0;
		for (auto obj : Manager::GetGameObjectList())
		{
			// TreeNodeで各オブジェクトを展開可能にする
			if (ImGui::TreeNode((void*)(intptr_t)index, "%d: %s", index, obj->GetName()))
			{
				// 座標（Position）の編集 (0.1f はドラッグ時の変化量)
				Vector3 pos = obj->GetPosition();
				if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) 
					obj->SetPosition(pos);

				// 回転（Rotation）の編集
				Vector3 rot = obj->GetRotation();
				if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
					obj->SetRotation(rot);

				// スケール（Scale）の編集
				Vector3 scale = obj->GetScale();
				if (ImGui::DragFloat3("Scale", &scale.x, 0.1f))
					obj->SetScale(scale);
			
				ImGui::TreePop(); // TreeNodeを閉じる
			}
			index++;
		}
		ImGui::Text("Total: %d", index); // 合計表示
	}

	// === 追加: カメラモードの切り替えUI ===
	if (ImGui::CollapsingHeader("Camera Settings"))
	{
		Camera* cam = Manager::GetCamera();
		if (cam)
		{
			// === 大元のカメラモードの切り替え ===
			int mode = (int)cam->GetMode();
			ImGui::Text("Camera Mode:");
			ImGui::RadioButton("Default", &mode, 0); ImGui::SameLine();
			ImGui::RadioButton("FPS", &mode, 1); ImGui::SameLine();
			ImGui::RadioButton("TPS", &mode, 2);
			cam->SetMode((CameraMode)mode);
			// 選択された値をカメラに反映
			cam->SetMode((CameraMode)mode);
		}
	}

	// ===== EnemyPool（独立管理）の状態 =====
	if (ImGui::CollapsingHeader("EnemyPool"))
	{
		ImGui::Text("Active: %d / %d",
			g_EnemyPool.GetActiveCount(),
			GameConfig::Game::SCORPION_NUM);

		for (auto obj : Manager::GetGameObjectList())
		{
			// GameObjectリストの中からEnemySpawnerを探す
			if (strcmp(obj->GetName(), "EnemySpawner") == 0)
			{
				EnemySpawner* spawner = static_cast<EnemySpawner*>(obj);

				// スライダーを表示 (1秒間に0体 ～ 5000体まで調整可能にする例)
				// GetSpawnRatePtr() で取得したアドレスを直接渡すことで、値を書き換えられます
				ImGui::SliderFloat("Spawn Rate (per sec)", spawner->GetSpawnRatePtr(), 0.0f, 1000.0f);

				// HP表示
				if (spawner->IsDestroyed())
				{
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Spawner: DESTROYED");
				}
				else
				{
					float ratio = spawner->GetHp() / GameConfig::Spawner::MAX_HP;
					ImGui::Text("Spawner HP:");
					ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
				}

				break; // 見つかったらループを抜ける
			}
		}
	}

	// ===== プレイヤーステータス =====
	if (ImGui::CollapsingHeader("Player"))
	{
		Player* player = Manager::GetGameObject<Player>();
		if (player)
		{
			float ratio = player->GetHp() / player->GetMaxHp();
			ImGui::ProgressBar(ratio, ImVec2(-1.0f, 0.0f));
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("HP %.0f / %.0f", player->GetHp(), player->GetMaxHp());
		}
	}

	// ===== 武器切り替えUI =====
	if (ImGui::CollapsingHeader("Weapon"))
	{
		Player* player = Manager::GetGameObject<Player>();
		if (player)
		{
			int index = player->GetWeaponIndex();
			auto& weapons = player->GetWeapons();

			// 所持武器をラジオボタンで一覧表示し、選択で切り替える
			for (int i = 0; i < (int)weapons.size(); i++)
			{
				if (ImGui::RadioButton(weapons[i]->GetName(), &index, i))
					player->SetWeaponIndex(i);
			}

			// 現在の武器の状態を表示する
			Weapon* w = player->GetWeapon();
			if (w)
			{
				ImGui::Separator();
				if (w->GetMagazineSize() == -1)
				{
					ImGui::Text("Ammo : Infinite");
				}
				else
				{
					ImGui::Text("Ammo : %d / %d", w->GetCurrentAmmo(), w->GetMagazineSize());
				}
				ImGui::Text(w->IsReloading() ? "Reloading..." : "Ready");
			}
		}
	}

	if (ImGui::CollapsingHeader("Damage Display"))
	{
		int dmode = (int)g_DamageVisualizer.GetMode();
		ImGui::RadioButton("Popup (hit position)", &dmode, 0); ImGui::SameLine();
		ImGui::RadioButton("Accumulate (bottom-right)", &dmode, 1);
		g_DamageVisualizer.SetMode((DamageDisplayMode)dmode);
	}

	// ===== パーティクルデバッグ =====
	if (ImGui::CollapsingHeader("Particle Debug"))
	{
		const Vector3 debugPos = { 0.0f, 3.0f, 0.0f };
		ImGui::Text("Spawn Position: (%.1f, %.1f, %.1f)", debugPos.x, debugPos.y, debugPos.z);

		auto& particleManager = ParticleManager::GetInstance();
		if (ImGui::Button("Explosion"))      particleManager.Emit(EffectType::Explosion,      debugPos);
		ImGui::SameLine();
		if (ImGui::Button("MuzzleFlash"))    particleManager.Emit(EffectType::MuzzleFlash,    debugPos);
		ImGui::SameLine();
		if (ImGui::Button("Hit"))            particleManager.Emit(EffectType::Hit,            debugPos);

		if (ImGui::Button("Smoke"))          particleManager.Emit(EffectType::Smoke,          debugPos);
		ImGui::SameLine();
		if (ImGui::Button("Spark"))          particleManager.Emit(EffectType::Spark,          debugPos);
		ImGui::SameLine();
		if (ImGui::Button("SpawnerDestroy")) particleManager.Emit(EffectType::SpawnerDestroy, debugPos);

		if (ImGui::Button("BossAppear"))     particleManager.Emit(EffectType::BossAppear,     debugPos);

		ImGui::SeparatorText("Full Explosion Sequence");
		// 発光フラッシュ→火球→火花→デブリ→煙→爆風リングを一括再生する
		if (ImGui::Button("Big Explosion")) particleManager.EmitBigExplosion(debugPos);
	}

	// =============== ここに移動！ ================
	InputVisualizer::Draw();
	// =============================================

	ImGui::End();
	} // g_ShowDebugUI

	// コライダーワイヤーフレーム（BackgroundDrawList に描くので必ず見える）
	if (g_ShowColliderDebug)
		g_ColliderDebugRenderer.Draw(g_CollisionManager, g_EnemyPool, GetCamera(), 2.5f);

	g_DamageVisualizer.Draw(Manager::GetCamera(),50);

	// クロスヘア（画面中央に固定）
	{
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float cx = SCREEN_WIDTH  * 0.5f;
		float cy = SCREEN_HEIGHT * 0.5f;
		const float len = 10.0f;
		const float gap =  3.0f;
		ImU32 col = IM_COL32(255, 255, 255, 220);
		dl->AddLine(ImVec2(cx - len - gap, cy), ImVec2(cx - gap, cy), col, 1.5f);
		dl->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + len + gap, cy), col, 1.5f);
		dl->AddLine(ImVec2(cx, cy - len - gap), ImVec2(cx, cy - gap), col, 1.5f);
		dl->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + len + gap), col, 1.5f);
	}

	// 爆発の画面フラッシュ（フェードより奥、他の描画より手前に表示する）
	ParticleManager::GetInstance().DrawScreenFlash();

	// フェードオーバーレイ（他のUIより手前に描画する）
	g_FadeManager.Draw();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Manager::DebugSystemInfo()
{
	ImGui::SeparatorText("System");

	ImGui::Text("FPS : %.1f",g_DebugInfo.FPS);

	ImGui::SliderFloat("Time Scale",&g_DebugInfo.timeScale,0.1f,10.0f);
	
	ImGui::Text("Frame Time : %.4f ms",g_DebugInfo.frameTime * 1000.0f);

	ImGui::Text("Accumulator : %.4f",g_DebugInfo.accumulator);

	ImGui::Text("Update Count : %d",g_DebugInfo.updateCount);

	ImGui::Text("Draw Count : %d",g_DebugInfo.drawCount);

	ImGui::Text("MaxUpdate Reached : %d",g_DebugInfo.maxUpdateReached);
}