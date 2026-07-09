#pragma once

// =====================================================
// トランジションシステムの使い方
// =====================================================
//
// ■ 基本の呼び出し方（これだけで動く）
//
//     g_TransitionManager.Play(TransitionType::Fade, TransitionMode::Out);
//
//   - Out : 通常表示 → 覆われた状態へ（画面を隠す）
//   - In  : 覆われた状態 → 通常表示へ（画面を見せる）
//   - duration を省略すると各トランジションのデフォルト時間になる。
//     秒数を指定したい場合は第3引数に渡す:
//       g_TransitionManager.Play(TransitionType::Fade, TransitionMode::Out, 1.0f);
//
// ■ 状態の確認
//
//     if (g_TransitionManager.IsFinished()) { ... } // 完了した"その1フレームだけ" true
//     if (g_TransitionManager.IsPlaying())  { ... } // 再生中かどうか
//
//   実際の使用例は sceneManager.cpp を参照（シーン切り替えのタイミング制御に使っている）。
//
// ■ 色や強さなどのパラメータを変えたい場合
//
//   GetTransition<T>() で実体を取得し、Play() の前に Set*** を呼ぶ。
//
//     if (auto* fade = g_TransitionManager.GetTransition<FadeTransition>(TransitionType::Fade))
//         fade->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 白フェードにする
//     g_TransitionManager.Play(TransitionType::Fade, TransitionMode::Out);
//
//   各クラスの Set*** は対応するヘッダー(例: fadeTransition.h)を参照。
//
// ■ 各トランジションの見た目とパラメータ（詳細は各ヘッダーのコメント参照）
//
//   Fade          : 単色へのフェード。          SetColor
//   Wipe          : 直線が画面を横切って覆う。   SetAngle, SetColor
//   Circle        : 中心点から円形に開閉する。   SetCenter, SetColor
//   Slide         : パネルが画面外からスライド。 SetDirection, SetColor
//   Curtain       : 左右2枚の幕が中央で開閉する。SetColor
//   Mosaic        : 実際の画面がブロック状に粗くなる（要デプス無しのライブ画面）。SetMaxBlockSize, SetColor
//   Blur          : 実際の画面がガウシアン風にぼける。                        SetMaxRadius, SetColor
//   Distortion    : 実際の画面が熱波状にノイズで歪む。                        SetStrength, SetScale, SetColor
//   PixelDissolve : ドット状のノイズパターンで覆っていく（ディザ溶暗）。       SetCellSize, SetColor
//
// ■ 新しいトランジションを追加する手順（switch文は増えない）
//
//   1. このファイルの TransitionType に種類を1行追加する
//   2. TransitionBase を継承した新クラスを作る（fadeTransition.h/.cpp 等を参考に）
//   3. transitionManager.cpp の Init() に Register<NewTransition>(TransitionType::New); を1行追加する
//
//   既存のクラス（TransitionManager本体・SceneManager・各シーン）は一切変更不要。
//
// =====================================================

// =====================================================
// TransitionType : トランジションの「種類」
// =====================================================
enum class TransitionType
{
    Fade,          // 単色フェード
    Wipe,          // 直線ワイプ
    Circle,        // 円形アイリス
    Slide,         // パネルスライド
    Curtain,       // 左右の幕
    Mosaic,        // モザイク化（ライブ画面加工）
    Blur,          // ガウシアンぼかし（ライブ画面加工）
    Distortion,    // ノイズ歪み（ライブ画面加工）
    PixelDissolve, // ドットディザ溶暗
};

// =====================================================
// TransitionMode : トランジションの「向き」
// In  = 覆われた状態 → 通常表示へ（見せる）
// Out = 通常表示 → 覆われた状態へ（隠す）
// =====================================================
enum class TransitionMode
{
    In,
    Out,
};
