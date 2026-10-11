// ゲーム全体で共有する定数
//   画面サイズ・レーンの配置・ノーツの大きさ・スクロール速度・キー割り当て。
//   見た目や操作感を調整したいときはまずここを変える。
#pragma once

#include <SDL3/SDL.h>

// 論理画面サイズ (px)。ウィンドウを拡大縮小してもこの解像度で描画される
constexpr int kScreenW = 480;
constexpr int kScreenH = 800;

// レーン数と 1 レーンの幅。レーン全体を画面中央に置くための左端 X 座標
constexpr int kLanes = 4;
constexpr float kLaneW = 90.0f;
constexpr float kFieldX = (kScreenW - kLaneW * kLanes) / 2.0f;

constexpr float kJudgeY = 680.0f;        // 判定ラインの Y 座標
constexpr float kNoteH = 18.0f;          // ノーツの高さ
constexpr double kScrollPxPerMs = 0.75;  // スクロール速度 (1 ms あたりに進む px)

constexpr double kJudgeFxMs = 450.0;     // 判定演出（光の粒・広がる枠・MISS の赤いにじみ）の長さ (ms)
constexpr double kPerfectGrowMs = 120.0; // PERFECT の文字が拡大しきるまでの時間 (ms)

// 各レーンに対応するキー（左から D F J K）
inline constexpr SDL_Keycode kLaneKeys[kLanes] = {SDLK_D, SDLK_F, SDLK_J, SDLK_K};
