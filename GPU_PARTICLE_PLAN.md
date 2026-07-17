# GPUパーティクル移行計画書

作成日: 2026-07-17 / 対象ブランチ: `GPUParticle`

## 実装結果（2026-07-17 実測・Release x64）

Step 0〜6 実装完了。実機検証済み:

| 条件 | Update CPU | Draw CPU | Draw Calls |
|---|---|---|---|
| CPUシミュレーション・10万粒子 | 2.288 ms | 1.821 ms | 1 |
| **GPUシミュレーション・10万粒子** | **0.012 ms** | **0.002 ms** | **2** |
| **GPUシミュレーション・100万粒子** | **2.183 ms** | 0.001 ms | 2 |

- 切替方法: デバッグUI「GPU Simulation」チェックボックス、またはゲーム中 `3` キー
- 100万粒子時もゲームは操作可能なまま（CPU版はフレーム崩壊する規模）
- GPU100万時の Update CPU 2.2ms は Dispatch発行＋統計読み戻しのコスト（粒子数に比例しない）
- 未実装（任意）: GPUタイムスタンプクエリによるCS実行時間の精密計測

## 1. 目的

現在のパーティクルシステムは **描画のみGPU（インスタンシング）、計算はCPU** で行っている。
これを **計算（生成・物理・寿命管理）もGPUに移し、フルGPUパーティクル** へ移行する。

### 現状のボトルネック（CPU側に残っている処理）

| 処理 | 場所 | コスト |
|---|---|---|
| 物理演算（乱流・減速・重力/浮力・地面バウンド・自転・寿命・色/サイズ補間） | `ParticleManager::Update` | 粒子数に比例（100万粒子で数十ms） |
| 描画リスト構築（バケット振り分け・インスタンスデータ組み立て・Map転送） | `ParticleRenderer::Draw` | 粒子数に比例＋毎フレーム最大48MBの転送 |

100万粒子時、CPUは毎フレーム全粒子を2回走査している。これがフレームレートの上限を決めている。

### 移行後の姿

- CPUの仕事は「**今フレームどこに何を何個出すか**」の指示（EmitRequest）を作るだけ（毎フレーム数件〜数十件）
- 生成・物理・描画リスト構築・描画数の決定まで **すべてGPU内で完結**（CPU⇔GPUの粒子データ転送ゼロ）
- 呼び出し側API（`Emit(EffectType, pos)` など）と全プリセットの見た目は **変更しない**

## 2. 全体アーキテクチャ

```
【CPU】                          【GPU】
ParticleEmitter                  ┌────────────────────────────┐
 （放出タイミング管理のみ）       │ ① Spawn CS                 │
   │                             │  EmitRequestを読み、        │
   │ EmitRequest[]               │  リングカーソルをatomicに   │
   │ （位置・数・プリセット値・   │  進めて粒子を初期化         │
   │   乱数シード）               │  （乱数はハッシュでGPU生成） │
   └──定数バッファで転送──────→ ├────────────────────────────┤
                                 │ ② Update CS                │
                                 │  1スレッド=1粒子で物理演算  │
                                 │  生存粒子をAppendBufferへ   │
                                 │  詰めて描画リストを構築     │
                                 ├────────────────────────────┤
                                 │ ③ DrawInstancedIndirect    │
                                 │  通常合成→加算合成の2回     │
                                 │  （描画数はGPUが自分で決定、 │
                                 │   CPUへの読み戻しなし）     │
                                 └────────────────────────────┘
```

### GPU上のリソース構成

| リソース | 型 | 内容 |
|---|---|---|
| 粒子プール | `RWStructuredBuffer<GPUParticle>` ×1 | 100万粒子の全状態。VRAM常駐（CPUからは触らない） |
| リングカーソル | `RWByteAddressBuffer` ×1 | 次の書き込み位置。Spawn CSが `InterlockedAdd` で進める |
| 描画リスト | `AppendStructuredBuffer<DrawData>` ×2 | 通常合成用・加算合成用。Update CSが生存粒子を詰める |
| 間接引数 | Indirect Args Buffer ×2 | `CopyStructureCount` でAppend数を書き込み、`DrawInstancedIndirect` が参照 |
| EmitRequest | 定数バッファ | 1リクエスト分のプリセット値＋位置＋数＋乱数シード |

**設計判断: フリーリスト（dead list）ではなくリングバッファを採用する。**
現行CPU実装もリングバッファ（古い粒子を上書き）であり、同じ意味論をatomicカーソル1本で再現できる。
Append/Consumeによる空きスロット管理より大幅に単純で、挙動も現行と一致する。

### GPUParticle構造体（1粒子 = 80バイト目標）

現行 `ParticleData`（約150B）から、GPU側では以下を圧縮する:

- `StartColor` / `EndColor` → `uint` ×2 にパック（R8G8B8A8）。Update CSが展開して補間
- `TexturePath`（ポインタ） → **テクスチャ配列のインデックス（uint）** に変更（後述）
- `Active` / `Additive` / `GroundAligned` / `GroundCollision` → 1つの `uint` にビットパック
- `Size` / `Color`（補間結果）→ 保持しない。Update CSが毎フレーム計算して描画リストに直接書く

100万粒子 × 80B = **80MB VRAM**（現行はメインメモリ150MB＋毎フレーム転送だったので、転送が消える分むしろ有利）

### テクスチャの扱い: Texture2DArray化

現行の「テクスチャ×ブレンドのバケット分け」はCPUの走査が前提の構造。GPU化にあたり:

- パーティクル用テクスチャ3種（`particle.png` / `smoke.png` / `white.png`）を起動時に **1つのTexture2DArrayへ統合**（サイズは最大のものに合わせてリサイズ）
- 粒子は「配列インデックス」を持ち、ピクセルシェーダーが `Texture2DArray.Sample(uv, index)` で読む
- これにより描画は **ブレンドモード2回のDrawInstancedIndirectだけ** になる（ドローコール最大6回→常に2回）

### 乱数のGPU化

生成時の個体差（方向・速さ・寿命・ジッター・乱流軸・自転・サイズばらつき）は、
`EmitRequest.seed + 粒子インデックス` を入力とする **ハッシュベース乱数（PCG/Wang hash）** でSpawn CS内に生成する。
CPUの `mt19937` と数列は変わるが、統計的な見た目は同一（プリセットの調整値はそのまま使える）。

## 3. 実装ステップ

コミット単位で区切れる7段階。各ステップ完了時点でビルド・動作が壊れないように進める。

### Step 0: 準備（半日）
- `Renderer` にコンピュートシェーダー作成ヘルパー `CreateComputeShader()` を追加
- vcxprojにHLSLのCS(5.0)ビルド設定を追加（既存VS/PSと同じ`shader\`出力）
- **確認**: 空のCSがDispatchできること

### Step 1: GPUリソース定義（半日）
- `particleGPU.h`: GPUParticle構造体・EmitRequest構造体（HLSL側と同レイアウト）
- `particleSystemGPU.h/.cpp`（新クラス）: バッファ群の確保・解放のみ実装
- テクスチャ3種のTexture2DArray統合ローダー
- **確認**: 起動時にリソース確保が成功すること（DebugレイヤーでのD3D11エラーなし）

### Step 2: Spawn CS（1日）
- `particleSpawnCS.hlsl`: リングカーソルatomic加算＋ハッシュ乱数で粒子初期化
- CPU側: `ParticleEmitter` の放出判定はそのまま流用し、`EmitOne` の代わりに
  「EmitRequest（プリセット値＋位置＋数＋シード）を積む」処理に差し替え
- 1フレームのリクエストは件数分 `Dispatch(ceil(count/256))` を発行（通常数件）
- **確認**: 後続ステップのUpdate/描画がまだ無いので、デバッグ用にプール先頭を読み戻して値を目視

### Step 3: Update CS（1〜2日）★物理移植の本丸
- `particleUpdateCS.hlsl`: 1スレッド=1粒子。`ParticleManager::Update` の物理をそのまま移植
  - 乱流（速度×軸の外積）／減速（Drag）／重力⇔浮力切替（BuoyancyDelay）
  - 地面バウンド（groundYは定数バッファで渡す。現行同様の平面判定）
  - 自転・寿命減算・サイズ/色の線形補間
- 生存粒子の描画データ（位置・サイズ・色・回転・テクスチャindex）を通常/加算の2本のAppendBufferへ詰める
- **確認**: この時点ではまだ描画は旧経路のため見た目は無し。タイムスタンプクエリでCS実行時間を計測

### Step 4: 描画切替（1日）
- `particleVS.hlsl` 改修: インスタンスストリーム入力をやめ、`StructuredBuffer` SRVを `SV_InstanceID` で読む方式へ
  （StructuredBufferは頂点バッファにバインドできないため。入力レイアウトは板ポリのみになる）
- `particlePS.hlsl`: Texture2DArray対応
- `CopyStructureCount` → `DrawInstancedIndirect` ×2（通常→加算の順。現状の描画順を維持）
- **確認**: 全プリセット（爆発・スコーピオン被弾・回復・ロケット・裂け目）の見た目が現行と同等であること

### Step 5: 統計とデバッグUI（半日）
- アクティブ数はAppendカウンタを staging バッファへコピーし **2フレーム遅れで読む**（GPUストールを作らない）
- デバッグUIに「Update GPU時間（タイムスタンプクエリ）」「Spawn数」を追加
- ストレステスト（Flood Sparks 10万/50万/100万）をEmitRequest経由で動くよう対応
- **確認**: 100万粒子時のフレームレートと各計測値

### Step 6: CPU/GPU切替スイッチと最終検証（半日）
- デバッグUIに **実行時トグル「Simulation: CPU / GPU」** を追加（旧経路は削除せず残す）
  - プレゼンで「同条件でCPU版○ms → GPU版○ms」を実演できることが就活上の最大の価値
- 検証項目:
  - [ ] 全エフェクトの見た目比較（CPU/GPU切替で違和感がないか）
  - [ ] Flood Sparks 100万でのFPS比較（CPU版の実測値を先に記録しておく）
  - [ ] 通常プレイ（数千粒子）で退行がないか（Dispatchの固定コストが問題にならないか）
  - [ ] シーン遷移をまたいだリソースの解放・再確保

**合計目安: 5〜6日**

## 4. 変更しないもの

- `Emit(EffectType, pos)` / `Emit(setting, pos)` / `EmitBigExplosion` などの呼び出し側API
- `ParticleSetting` と全プリセットの調整値（見た目の互換を保つ）
- 画面フラッシュ・Explosionゲームオブジェクト（別系統）
- 天候システム（`WeatherManager`。独立したエミッタなので今回は対象外）
- ソートなし描画（現行も深度無効＋通常→加算の順で描くだけ。今回も同じ）

## 5. リスクと対応

| リスク | 対応 |
|---|---|
| 描画順の変化: 現行は「プール順」だがAppendBufferは順序不定 | 深度テスト無効＋加算合成が主体のため視覚差はほぼ出ない見込み。Step 4で全プリセット目視比較し、問題があれば通常合成のみ許容順に検討 |
| テクスチャ配列化でサイズの異なるPNGを統合 | 最大サイズに合わせてCPUでリサイズ（DirectXTexの`Resize`）。見た目確認 |
| 乱数列の変化で微妙に印象が変わる | プリセット値は同じなので統計的分布は同一。気になる場合のみ個別調整 |
| GPUデバッグの難しさ（CSの中は見えない） | ステップごとに読み戻し確認を入れる。RenderDocでの検証手順も残す |
| 通常プレイ時（数千粒子）にDispatch固定コストで逆に遅くなる | Update CSは常に全プール走査ではなく、現行同様ハイウォーターマーク相当の範囲をCPUが管理してDispatch数を絞る |

## 6. プレゼンでの説明ポイント（就活用）

1. **Before/Afterを同一画面で切替デモ**: 「CPUシミュレーション○ms → GPU化で○ms」（Step 6のトグル）
2. 段階的な高速化のストーリーが完成する:
   「1粒子1ドロー → ①インスタンシングで描画をGPU化 → ②計算もGPU化してCPU⇔GPU転送を排除」
3. 技術キーワード: コンピュートシェーダー / StructuredBuffer / atomic操作 / AppendBuffer / DrawInstancedIndirect（間接描画）/ GPU駆動レンダリング

## 7. 決定事項・確認したいこと

- **推奨**: CPU経路は削除せず切替式で残す（比較デモ・保険のため）→ 問題なければこの方針で進める
- ソートなし（現行同様）でよいか → 現行と同じ見た目なので対象外とする
- 着手はAchievementOptionsブランチのコミット・マージ後、`GPUParticle` ブランチを切って行う
