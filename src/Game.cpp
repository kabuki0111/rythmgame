// Game の進行処理: 初期化・リスタート・キー入力・毎フレームの更新・判定
#include "Game.h"

#include <algorithm>
#include <cmath>

bool Game::init(const std::string& assetDir) {
    // 曲 (song.wav) と譜面 (chart.txt) を読み込む
    if (!clock_.open((assetDir + "song.wav").c_str())) return false;
    chart_ = loadChart(assetDir + "chart.txt");
    if (chart_.empty()) {
        SDL_Log("譜面が空です: %schart.txt", assetDir.c_str());
        return false;
    }
    restart();
    return true;
}

void Game::restart() {
    notes_ = chart_;  // 元の譜面をコピーし直して done フラグをリセット
    stats_ = {};
    lastJudgeText_ = "";
    judgeFx_.clear();
    finished_ = false;
    clock_.start();
}

void Game::onKeyDown(const SDL_KeyboardEvent& key) {
    if (key.repeat) return;  // 押しっぱなしによるキーリピートは無視

    // 機能キー
    switch (key.key) {
    case SDLK_UP: offsetMs_ += 5.0; return;
    case SDLK_DOWN: offsetMs_ -= 5.0; return;
    case SDLK_A: autoplay_ = !autoplay_; return;
    case SDLK_R: restart(); return;
    default: break;
    }

    // レーンキー
    for (int lane = 0; lane < kLanes; ++lane) {
        if (key.key == kLaneKeys[lane]) {
            laneHeld_[lane] = true;
            // 押した瞬間のタイムスタンプで判定する（フレーム待ちの遅れを含めない）
            hit(lane, songTime(key.timestamp));
        }
    }
}

void Game::onKeyUp(const SDL_KeyboardEvent& key) {
    // レーンキーを離したら押下中の表示を解除し、ロングノーツの終点を判定する
    for (int lane = 0; lane < kLanes; ++lane) {
        if (key.key == kLaneKeys[lane]) {
            laneHeld_[lane] = false;
            release(lane, songTime(key.timestamp));
        }
    }
}

void Game::update() {
    clock_.update();
    const double t = songTime(SDL_GetTicksNS());

    // 未判定のノーツを処理: オートプレイなら時刻ちょうどで PERFECT、
    // そうでなければ GOOD の判定幅を過ぎたものを MISS にする
    for (Note& n : notes_) {
        if (n.done) continue;
        if (n.holding) {
            // ロングノーツを終点まで押し続けたら終点も PERFECT
            if (t >= n.endMs) judgeTail(n, Judge::Perfect);
        } else if (autoplay_ && t >= n.timeMs) {
            judgeHead(n, Judge::Perfect);
        } else if (t - n.timeMs > kGoodMs) {
            judgeHead(n, Judge::Miss);  // 見逃し
        }
    }

    // ヒット時のレーンの光を少しずつ弱める
    for (float& f : laneFlash_) f = std::max(0.0f, f - 0.06f);

    // 表示し終えた判定演出を取り除く
    judgeFx_.erase(std::remove_if(judgeFx_.begin(), judgeFx_.end(),
                                  [t](const JudgeFx& fx) { return t - fx.startMs > kJudgeFxMs; }),
                   judgeFx_.end());

    // 曲の長さを過ぎたらリザルト表示へ
    if (!finished_ && t > clock_.durationMs()) finished_ = true;
}

void Game::hit(int lane, double t) {
    // そのレーンで判定幅内にある、最も早い未処理ノーツを探す
    // （押し続けているロングノーツは始点の判定が済んでいるので対象外）
    for (Note& n : notes_) {
        if (n.done || n.holding || n.lane != lane) continue;
        if (n.timeMs - t > kGoodMs) return;  // まだ遠い（空押し）
        const Judge j = judgeByDiff(std::fabs(t - n.timeMs));
        if (j != Judge::Miss) return judgeHead(n, j);
    }
}

void Game::release(int lane, double t) {
    if (autoplay_) return;  // オートプレイ中は終点まで押し続ける扱い
    for (Note& n : notes_) {
        if (!n.holding || n.lane != lane) continue;
        // 終点とのずれで判定する。GOOD の幅より早く離したら MISS
        return judgeTail(n, judgeByDiff(std::fabs(n.endMs - t)));
    }
}

void Game::judgeHead(Note& n, Judge j) {
    applyJudge(n.lane, j);
    if (n.isLong() && j != Judge::Miss) {
        n.holding = true;  // 終点まで押し続ける
        return;
    }
    if (n.isLong()) applyJudge(n.lane, Judge::Miss);  // 始点を逃したら終点も MISS
    n.done = true;  // 判定済みにして以降の判定・描画から外す
}

void Game::judgeTail(Note& n, Judge j) {
    n.holding = false;
    n.done = true;
    applyJudge(n.lane, j);
}

void Game::applyJudge(int lane, Judge j) {
    // 判定が出た瞬間を演出の開始時刻にする
    // （MISS はノーツ時刻の 100ms 後に確定するので、ノーツ時刻を基準にすると演出の出だしが欠ける）
    const double now = time();

    // 画面に出す判定表示を更新し、成績に加算する
    lastJudgeTime_ = now;
    lastJudgeText_ = judgeName(j);
    lastJudge_ = j;
    stats_.add(j);
    if (j != Judge::Miss) laneFlash_[lane] = 1.0f;  // ヒットしたレーンを光らせる
    judgeFx_.push_back({lane, j, now, fxSeq_++});   // 判定ごとの演出を出す
}
