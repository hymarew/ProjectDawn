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
#include "transitionManager.h"
#include "saveManager.h"
#include "particleManager.h"
#include "soundManager.h"
#include "spaceRiftDebug.h"
#include "weatherManager.h"
#include "gameContext.h"
#include "dropManager.h"
#include "dynamicLightManager.h"

//インスタンス
std::list<GameObject*> Manager::m_GameObject;
ShadowRenderer* g_ShadowRenderer = nullptr;

BulletPool       g_BulletPool;
BulletManager    g_BulletManager;
CollisionManager g_CollisionManager;
bool g_CastShadow        = true;
bool g_ShowDebugUI       = false;
bool g_ShowColliderDebug = false;

// ロケット演出のデバッグトグル（影ちかつき等の原因切り分け用。全てONが通常状態）
bool g_RocketSparkEnabled    = true;
bool g_RocketMuzzleEnabled   = true;
bool g_RocketLightEnabled    = true;
bool g_ExplosionLightEnabled = true;
bool g_DynamicLightsEnabled  = true;
Camera* Manager::m_Camera = nullptr;

// ---------------------------------------------------------
// トランジションデバッグボタンの自動往復（Out再生→覆いきる→0.5秒待機→In再生）
// ---------------------------------------------------------
namespace
{
    enum class DebugTransitionState { Idle, WaitingForCover, DelayBeforeReveal };

    DebugTransitionState s_DebugTransitionState = DebugTransitionState::Idle;
    TransitionType       s_DebugTransitionType  = TransitionType::Fade;
    float                s_DebugReturnTimer     = 0.0f;
    constexpr float       DEBUG_RETURN_DELAY    = 0.5f; // 覆いきってからInを再生するまでの待ち時間（秒）

    void PlayDebugTransition(TransitionType type)
    {
        g_TransitionManager.Play(type, TransitionMode::Out);
        s_DebugTransitionType  = type;
        s_DebugTransitionState = DebugTransitionState::WaitingForCover;
    }

    void UpdateDebugTransitionAutoReturn(float dt)
    {
        switch (s_DebugTransitionState)
        {
        case DebugTransitionState::WaitingForCover:
            // Out再生が終わる（画面を覆いきる）まで待つ
            if (!g_TransitionManager.IsPlaying())
            {
                s_DebugTransitionState = DebugTransitionState::DelayBeforeReveal;
                s_DebugReturnTimer     = DEBUG_RETURN_DELAY;
            }
            break;

        case DebugTransitionState::DelayBeforeReveal:
            s_DebugReturnTimer -= dt;
            if (s_DebugReturnTimer <= 0.0f)
            {
                g_TransitionManager.Play(s_DebugTransitionType, TransitionMode::In);
                s_DebugTransitionState = DebugTransitionState::Idle;
            }
            break;

        default:
            break;
        }
    }
}

void Manager::Init()
{
	Renderer::Init();
	g_ShadowRenderer = new ShadowRenderer();
	g_ShadowRenderer->Init();
	InputManager::Init();
	Audio::InitMaster();
	g_SoundManager.Init();
	g_SpaceRiftDebug.Init();

	// トランジション（Fade等）のGPUリソースを用意する。
	// SceneManager::Init() が起動直後にFadeInを再生するため、それより前に呼ぶ必要がある。
	g_TransitionManager.Init();

	// 動的ポイントライト（ロケットの噴射炎・爆発フラッシュ等）のGPUリソースを用意する
	g_DynamicLightManager.Init();

	// セーブデータをロードする。ファイルがなければデフォルト値で新規生成して保存する。
	// SceneManager::Init より前に呼ぶことで、初回シーン（TitleScene）表示前にデータが揃う。
	g_SaveManager.Load();

	// セーブ済みオプション設定（音量・マウス感度）を各システムへ反映する。
	// g_SoundManager.Init() / InputManager::Init() より後、かつ Load() 直後に行う
	{
		using namespace GameConfig::Options;
		g_SoundManager.SetBgmVolume(g_SaveManager.GetSettingFloat(KEY_BGM_VOLUME, DEFAULT_VOLUME));
		g_SoundManager.SetSEVolume (g_SaveManager.GetSettingFloat(KEY_SE_VOLUME,  DEFAULT_VOLUME));
		InputManager::SetMouseSensitivityScale(
			g_SaveManager.GetSettingFloat(KEY_SENSITIVITY, DEFAULT_SENSITIVITY));
	}

	// シーン管理を初期化（TitleScene からスタート）
	g_SceneManager.Init();
}


void Manager::Uninit()
{
	g_SceneManager.Uninit();

	g_TransitionManager.Uninit();
	g_DynamicLightManager.Uninit();

	g_ShadowRenderer->Uninit();
	delete g_ShadowRenderer;
	g_SpaceRiftDebug.Uninit();
	g_SoundManager.Uninit();
	Audio::UninitMaster();

	Renderer::Uninit();
}

void Manager::Update(float dt)
{
	InputManager::Update();

	// 現在のシーンを更新
	g_SceneManager.Update(dt);

	// 動的ポイントライト（寿命付きの爆発フラッシュ等）の時間管理
	g_DynamicLightManager.Update(dt);

	// トランジションデバッグボタンの自動往復（Out完了→0.5秒待機→In）
	UpdateDebugTransitionAutoReturn(dt);

	// SpaceRiftデバッグデモ（Spawn済みの間だけ時間経過・パーティクル放出を進める）
	g_SpaceRiftDebug.Update(dt);
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
			auto& equip = player->GetEquip();

			// 装備スロットの表示（ステージ中の装備変更は不可のため読み取り専用。
			// アクティブ切替はホイール = WeaponEquip::SwitchSlot のみ）
			for (int i = 0; i < (int)EquipSlot::Count; i++)
			{
				Weapon* w = equip.GetWeapon((EquipSlot)i);
				bool active = ((EquipSlot)i == equip.GetActiveSlot());
				ImGui::Text("%s %-9s : %s",
					active ? ">" : " ",
					EquipSlotToString((EquipSlot)i), w ? w->GetName() : "(none)");
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

		// ---- シミュレーション実行先の切り替え（CPU/GPU比較デモ用） ----
		// GPU: 生成・物理・描画リスト構築・描画数決定まで全てコンピュートシェーダー。
		//      CPUの仕事はEmitRequestの発行のみで、Update CPUがほぼ0になるのが見どころ
		ImGui::SeparatorText("Simulation");
		{
			bool useGPU = particleManager.IsUseGPU();
			if (ImGui::Checkbox("GPU Simulation (Compute Shader)", &useGPU))
				particleManager.SetUseGPU(useGPU);
		}

		// ---- 統計表示 ----
		// 旧実装は「1パーティクル=1ドローコール」だったため、
		// Active数に対してDraw Callsが数回で済んでいることが効果の証明になる
		ImGui::SeparatorText(particleManager.IsUseGPU() ? "GPU Particle Stats" : "GPU Instancing Stats");
		{
			const ParticleStats& stats = particleManager.GetStats();
			ImGui::Text("Active Particles : %d / %d", stats.ActiveCount, GameConfig::Particle::POOL_SIZE);
			ImGui::Text("Scan Range       : %d", stats.UsedSlots); // 走査範囲（粒子が消えると自動で縮む）
			ImGui::Text("Draw Calls       : %d", stats.DrawCalls);
			ImGui::Text("Update CPU       : %.3f ms", stats.UpdateMs);
			ImGui::Text("Draw   CPU       : %.3f ms", stats.DrawMs);
		}

		// ---- ストレステスト ----
		// 大量パーティクルへの耐性を実演するための負荷試験。
		ImGui::SeparatorText("Stress Test");
		{
			// 爆発一式（フラッシュ・火球・火花・デブリ・煙・リング）を周囲へ一斉発生
			static int stressExplosions = 10;
			ImGui::SliderInt("Explosions", &stressExplosions, 1, 50);
			if (ImGui::Button("Burst Explosions"))
			{
				for (int i = 0; i < stressExplosions; i++)
				{
					Vector3 pos = debugPos;
					pos.x += (rand() / (float)RAND_MAX - 0.5f) * 40.0f;
					pos.z += (rand() / (float)RAND_MAX - 0.5f) * 40.0f;
					particleManager.EmitBigExplosion(pos);
				}
			}

			// プールを一気に埋める火花の洪水（純粋な描画数の上限テスト）
			// 選んだ個数を5ヶ所に分けてバースト放出する（寿命は3〜6秒に延長）
			static int floodCount = 100000;
			ImGui::RadioButton("10万",  &floodCount, 100000);  ImGui::SameLine();
			ImGui::RadioButton("50万",  &floodCount, 500000);  ImGui::SameLine();
			ImGui::RadioButton("100万", &floodCount, 1000000);
			if (ImGui::Button("Flood Sparks"))
			{
				ParticleSetting s = ParticlePreset::Spark();
				s.BurstCount = floodCount / 5;
				s.MinLife    = 3.0f;
				s.MaxLife    = 6.0f;
				for (int i = 0; i < 5; i++)
				{
					Vector3 pos = debugPos;
					pos.x += (rand() / (float)RAND_MAX - 0.5f) * 60.0f;
					pos.y += 5.0f;
					pos.z += (rand() / (float)RAND_MAX - 0.5f) * 60.0f;
					particleManager.Emit(s, pos);
				}
			}
		}

		ImGui::SeparatorText("Effect Presets");
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

		ImGui::SeparatorText("Scorpion Armor FX");
		// 装甲被弾/撃破演出（火花・装甲片・粉・衝撃リングの合成。フラッシュは実際の被弾時のみ）
		if (ImGui::Button("Scorpion Hit"))   particleManager.EmitScorpionHit(debugPos);
		ImGui::SameLine();
		if (ImGui::Button("Scorpion Death")) particleManager.EmitScorpionDeath(debugPos);

		ImGui::SeparatorText("Rocket FX Debug (flicker isolation)");
		// 影のちかつき原因を切り分けるため、ロケット演出を個別にON/OFFできる
		ImGui::Checkbox("Sparks",              &g_RocketSparkEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Muzzle Flash",        &g_RocketMuzzleEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Rocket Light",        &g_RocketLightEnabled);
		ImGui::Checkbox("Explosion Light",     &g_ExplosionLightEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Dynamic Lights (ALL)", &g_DynamicLightsEnabled);

		ImGui::SeparatorText("World Item");
		if (ImGui::Button("Heal FX")) particleManager.EmitHeal(debugPos);
		ImGui::SameLine();
		// プレイヤーの前方3mに回復アイテムを配置する（取得テスト用）
		if (ImGui::Button("Spawn Heal"))
		{
			Player* p = Manager::GetGameObject<Player>();
			if (p)
			{
				Vector3 pos = p->GetPosition();
				pos.z += 3.0f;
				GameContext::Instance().itemFactory.Spawn(ItemID(2001), pos);
			}
		}
		ImGui::SameLine();
		// ドロップ演出テスト: プレイヤー前方でスコーピオンのドロップ抽選を10回走らせる
		if (ImGui::Button("Test Drop x10"))
		{
			Player* p = Manager::GetGameObject<Player>();
			if (p)
			{
				Vector3 pos = p->GetPosition();
				pos.z += 5.0f;
				for (int i = 0; i < 10; i++)
					g_DropManager.OnEnemyKilled("Scorpion", pos);
			}
		}
	}

	// ===== SpaceRift(空間の裂け目)デバッグ =====
	// パネルの中身（Spawn/Destroy・パラメータ調整）は SpaceRiftDebugPanel が担当する
	g_SpaceRiftDebug.DrawImGui();

	// ===== 天候(雨・雪)デバッグ =====
	// パネルの中身（ON/OFF・各種パラメータ調整）は WeatherManager が担当する
	WeatherManager::GetInstance().DrawImGui();

	// ===== トランジションデバッグ =====
	// ボタン1つで Out再生→覆いきる→0.5秒待機→In再生 まで自動で行う。
	// （Out単体だと画面が覆われたままになりデバッグUIが隠れて操作できなくなるため）
	if (ImGui::CollapsingHeader("Transition Debug"))
	{
		ImGui::Text("State: %s", g_TransitionManager.IsPlaying() ? "Playing" : "Idle");

		if (ImGui::Button("Fade"))          PlayDebugTransition(TransitionType::Fade);
		ImGui::SameLine();
		if (ImGui::Button("Wipe"))          PlayDebugTransition(TransitionType::Wipe);
		ImGui::SameLine();
		if (ImGui::Button("Circle"))        PlayDebugTransition(TransitionType::Circle);
		ImGui::SameLine();
		if (ImGui::Button("Slide"))         PlayDebugTransition(TransitionType::Slide);

		if (ImGui::Button("Curtain"))       PlayDebugTransition(TransitionType::Curtain);
		ImGui::SameLine();
		if (ImGui::Button("Mosaic"))        PlayDebugTransition(TransitionType::Mosaic);
		ImGui::SameLine();
		if (ImGui::Button("Blur"))          PlayDebugTransition(TransitionType::Blur);
		ImGui::SameLine();
		if (ImGui::Button("Distortion"))    PlayDebugTransition(TransitionType::Distortion);

		if (ImGui::Button("PixelDissolve")) PlayDebugTransition(TransitionType::PixelDissolve);
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

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// トランジション（Fade等）オーバーレイ。
	// ImGui描画の後に呼ぶことで、ポーズメニュー等の全UIより手前に確実に描画される。
	// (Transitionは独自シェーダーを使う raw な D3D 描画のため、ImGuiのDrawListより後段になる)
	g_TransitionManager.Draw();
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