// ゲーム本体: 入力・判定・進行は Game.cpp、描画は GameRender.cpp に実装
#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "AudioClock.h"
#include "Chart.h"
#include "Config.h"
#include "Judge.h"

class Game {
public:
    // 曲と譜面を assetDir から読み込んでプレイを開始する。失敗したら false
    bool init(const std::string& assetDir);

    // 譜面・成績を初期状態に戻し、曲を先頭から再生し直す
    void restart();

    void shutdown() { clock_.close(); }
    void setAutoplay(bool on) { autoplay_ = on; }
    double time() const { return songTime(SDL_GetTicksNS()); }  // オフセット補正後の現在時刻 (ms)

    // キー入力の処理（レーンキーの判定、オフセット調整、オートプレイ、リスタート）
    void onKeyDown(const SDL_KeyboardEvent& key);
    void onKeyUp(const SDL_KeyboardEvent& key);

    // 毎フレームの進行処理（時計の更新、見逃し MISS、ロングノーツの終点、オートプレイ、終了判定）
    void update();

    // 毎フレームの描画（GameRender.cpp）
    void render(SDL_Renderer* r);

private:
    // SDL タイムスタンプ (ns) を、判定オフセットを差し引いた曲の時刻 (ms) に変換する
    double songTime(Uint64 ticksNs) const { return clock_.timeAt(ticksNs) - offsetMs_; }

    // レーン lane を時刻 t に叩いたときの判定
    void hit(int lane, double t);

    // レーン lane のキーを時刻 t に離したときの判定（押し続けているロングノーツの終点）
    void release(int lane, double t);

    // ノーツ（ロングノーツでは始点）に判定 j を付ける。
    // ロングノーツの始点を叩けたら押し続け状態に、逃したら終点も MISS にする
    void judgeHead(Note& n, Judge j);

    // 押し続けているロングノーツの終点に判定 j を付ける
    void judgeTail(Note& n, Judge j);

    // 判定 1 回分を成績と演出（判定表示・レーンの光・判定演出）に反映する
    void applyJudge(int lane, Judge j);

    // 曲の終了後に重ねて表示するリザルト画面（GameRender.cpp）
    void renderResult(SDL_Renderer* r);

    // 判定ごとの演出（光の粒・広がる枠・MISS の赤いにじみ）を描く（GameRender.cpp）
    void renderJudgeFx(SDL_Renderer* r, double t);

    // 判定演出 1 回分
    struct JudgeFx {
        int lane;        // 演出を出すレーン
        Judge judge;     // 判定（演出の種類と色を決める）
        double startMs;  // 演出の開始時刻（曲の時刻）
        Uint32 seed;     // 光の粒の飛び方を決める乱数の種
    };

    AudioClock clock_;
    std::vector<Note> chart_;           // 読み込んだ元の譜面（リスタート用）
    std::vector<Note> notes_;           // プレイ中の譜面（done フラグが変化する）
    JudgeStats stats_;
    bool laneHeld_[kLanes] = {};        // 各レーンのキーを押しているか
    float laneFlash_[kLanes] = {};      // ヒット時の光の強さ (1.0 → 0 へ減衰)
    double offsetMs_ = 0.0;             // 判定オフセット（↑↓キーで ±5ms）
    bool autoplay_ = false;
    bool finished_ = false;             // 曲が終わってリザルト表示中か
    std::string lastJudgeText_;         // 直前の判定名（画面表示用）
    Judge lastJudge_ = Judge::Miss;     // 直前の判定（文字の色と動きの切り替え用）
    double lastJudgeTime_ = 0.0;        // 直前の判定が出た時刻
    std::vector<JudgeFx> judgeFx_;      // 表示中の判定演出
    Uint32 fxSeq_ = 0;                  // 判定演出の通し番号（乱数の種に使う）
};
