// Game の描画（レーン・ノーツ・HUD・リザルト画面）
//   毎フレーム画面全体を描き直す。ノーツの位置は曲の時刻から毎回計算する。
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "Game.h"

namespace {

// 文字列 s を拡大率 scale で描いたとき、画面中央に来る X 座標（デバッグフォントは 1 文字 8px）
float centerX(const char* s, float scale) {
    return (kScreenW / scale - (float)std::strlen(s) * 8.0f) / 2.0f;
}

// 種 seed と番号 i から 0〜1 の疑似乱数を作る（同じ入力なら毎フレーム同じ値）
float hash01(Uint32 seed, int i) {
    Uint32 h = seed * 747796405u + (Uint32)i * 2891336453u;
    h ^= h >> 16;
    h *= 2246822519u;
    h ^= h >> 13;
    return (float)(h & 0xFFFF) / 65535.0f;
}

// 判定ごとの演出の見た目。判定が良いほど派手にする
struct JudgeStyle {
    Uint8 r, g, b;    // 演出と判定文字の色
    int particles;    // 光の粒の数（0 なら粒なし）
    float ringGrow;   // 広がる枠の大きさ（0 なら枠なし）
};

const JudgeStyle& styleOf(Judge j) {
    static const JudgeStyle kStyles[] = {
        {255, 215, 90, 14, 1.0f},  // PERFECT: 金・粒多め（文字は拡大する）
        {110, 210, 255, 8, 0.7f},  // GREAT  : 水色・粒控えめ
        {120, 230, 130, 0, 0.4f},  // GOOD   : 緑・小さな枠だけ
        {255, 70, 70, 0, 0.0f},    // MISS   : 赤・レーンが赤くにじむ（文字は震える）
    };
    return kStyles[(int)j];
}

constexpr double kMissShakeMs = 300.0;  // MISS の文字が震える時間 (ms)

// PERFECT の文字の拡大率。小さく出て勢いよく広がり、少し行き過ぎてから落ち着く
//   0 → kPerfectGrowMs            : 1.5 倍 → 3.3 倍（出だしが速い）
//   kPerfectGrowMs → +kSettleMs   : 3.3 倍 → 3.0 倍
//   それ以降                       : 3.0 倍のまま
float perfectTextScale(float age) {
    constexpr float kFrom = 1.5f, kPeak = 3.3f, kRest = 3.0f;
    constexpr float kSettleMs = 100.0f;
    const float grow = (float)kPerfectGrowMs;
    if (age < grow) {
        const float q = 1.0f - age / grow;
        return kFrom + (kPeak - kFrom) * (1.0f - q * q);
    }
    if (age < grow + kSettleMs) return kPeak - (kPeak - kRest) * (age - grow) / kSettleMs;
    return kRest;
}

}  // namespace

void Game::renderJudgeFx(SDL_Renderer* r, double t) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
    for (const JudgeFx& fx : judgeFx_) {
        const float age = (float)(t - fx.startMs);
        if (age < 0.0f) continue;
        const float p = age / (float)kJudgeFxMs;  // 進み具合 0 → 1
        if (p >= 1.0f) continue;
        const JudgeStyle& s = styleOf(fx.judge);
        const float fade = 1.0f - p;
        const float laneX = kFieldX + fx.lane * kLaneW;
        const float cx = laneX + kLaneW / 2;  // 演出の中心（判定ライン上のレーン中央）

        // MISS: 判定ラインから上へ、赤いにじみをグラデーションで重ねる（上ほど薄い）
        if (fx.judge == Judge::Miss) {
            constexpr int kBands = 8;
            constexpr float kBandH = 20.0f;
            for (int i = 0; i < kBands; ++i) {
                const float a = fade * 110.0f * (1.0f - (float)i / kBands);
                SDL_SetRenderDrawColor(r, s.r, s.g, s.b, (Uint8)a);
                SDL_FRect band{laneX, kJudgeY - kBandH * (i + 1), kLaneW - 2, kBandH};
                SDL_RenderFillRect(r, &band);
            }
            continue;
        }

        const Uint8 alpha = (Uint8)(fade * 255);
        SDL_SetRenderDrawColor(r, s.r, s.g, s.b, alpha);

        // 広がって消えていく枠: 出だしは速く、だんだんゆっくり広がる
        if (s.ringGrow > 0.0f) {
            const float ease = 1.0f - fade * fade;
            const float w = (kLaneW - 10) * (1.0f + ease * 0.8f * s.ringGrow);
            const float h = kNoteH * (1.0f + ease * 2.0f * s.ringGrow);
            for (float k = 0; k < 3; ++k) {  // 3 重に描いて太い枠にする
                SDL_FRect ring{cx - w / 2 - k, kJudgeY - h / 2 - k, w + k * 2, h + k * 2};
                SDL_RenderRect(r, &ring);
            }
        }

        // 光の粒: 判定ラインから上向きに飛び散り、重力で落ちながら消える
        for (int i = 0; i < s.particles; ++i) {
            const float angle = -3.14159f * (0.08f + 0.84f * hash01(fx.seed, i * 2));  // 真横を除いた上半分
            const float speed = 0.25f + 0.4f * hash01(fx.seed, i * 2 + 1);            // px/ms
            const float px = cx + std::cos(angle) * speed * age;
            const float py = kJudgeY + std::sin(angle) * speed * age + 0.0012f * age * age / 2;
            const float size = 5.0f * fade + 1.0f;
            SDL_FRect dot{px - size / 2, py - size / 2, size, size};
            SDL_RenderFillRect(r, &dot);
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void Game::render(SDL_Renderer* r) {
    const double t = songTime(SDL_GetTicksNS());

    // 背景を塗りつぶす
    SDL_SetRenderDrawColor(r, 16, 16, 24, 255);
    SDL_RenderClear(r);

    // レーン: 内側 2 レーンは少し明るく、キーを押している間はさらに明るくする
    for (int lane = 0; lane < kLanes; ++lane) {
        const float x = kFieldX + lane * kLaneW;
        const Uint8 base = (lane == 1 || lane == 2) ? 34 : 28;
        const Uint8 hold = laneHeld_[lane] ? 22 : 0;
        
        SDL_SetRenderDrawColor(r, base + hold, base + hold, base + 12 + hold, 255);
        
        SDL_FRect bg{x, 0, kLaneW - 2, (float)kScreenH};
        
        SDL_RenderFillRect(r, &bg);

        // ヒット直後は判定ラインの上を加算合成で光らせる（強さは update() で減衰）
        if (laneFlash_[lane] > 0.0f) {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(r, 255, 210, 90, (Uint8)(laneFlash_[lane] * 160));
            
            SDL_FRect fl{x, kJudgeY, kLaneW - 2, 220};
            
            SDL_RenderFillRect(r, &fl);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }
    }

    // 判定ライン
    SDL_SetRenderDrawColor(r, 240, 240, 255, 255);
    
    SDL_FRect line{kFieldX, kJudgeY - 2, kLaneW * kLanes, 4};
    
    SDL_RenderFillRect(r, &line);

    // ノーツ: 判定ラインからの距離 = (ノーツ時刻 − 現在時刻) × 速度
    for (const Note& n : notes_) {
        if (n.done) continue;

        // 押し続けているロングノーツの始点は判定ラインに留める
        const float y = n.holding ? kJudgeY : kJudgeY - (float)((n.timeMs - t) * kScrollPxPerMs);
        // ロングノーツの終点（通常ノーツは始点と同じ）
        const float tailY = n.isLong() ? kJudgeY - (float)((n.endMs - t) * kScrollPxPerMs) : y;

        if (y < -kNoteH) break;  // ソート済みなのでこれ以降は画面外
        if (tailY > kScreenH + kNoteH) continue;

        // 内側 2 レーンは水色、外側 2 レーンは白
        const bool inner = (n.lane == 1 || n.lane == 2);
        const Uint8 red = inner ? 90 : 240;
        const Uint8 green = inner ? 200 : 240;
        const float laneX = kFieldX + n.lane * kLaneW;

        // ロングノーツの帯（始点〜終点）と終点の印。押し続けている間は帯を明るくする
        if (n.isLong()) {
            const Uint8 dim = n.holding ? 1 : 2;
            SDL_SetRenderDrawColor(r, red / dim, green / dim, 255 / dim, 255);
            SDL_FRect body{laneX + 16, tailY, kLaneW - 34, y - tailY};
            SDL_RenderFillRect(r, &body);

            SDL_SetRenderDrawColor(r, red, green, 255, 255);
            SDL_FRect tail{laneX + 4, tailY - kNoteH / 4, kLaneW - 10, kNoteH / 2};
            SDL_RenderFillRect(r, &tail);
        }

        SDL_SetRenderDrawColor(r, red, green, 255, 255);

        SDL_FRect rect{laneX + 4, y - kNoteH / 2, kLaneW - 10, kNoteH};

        SDL_RenderFillRect(r, &rect);
    }

    // 判定ごとの演出（ノーツより手前に重ねる）
    renderJudgeFx(r, t);

    // テキスト（SDL3 内蔵のデバッグフォント, 8x8）
    // 2 倍サイズ: スコア・コンボ数・直前の判定（判定は 500ms だけ表示）
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    
    char buf[128];
    std::snprintf(buf, sizeof buf, "SCORE %07ld", stats_.score);
    
    SDL_RenderDebugText(r, 8, 8, buf);
    if (stats_.combo > 1 && !finished_) {
        std::snprintf(buf, sizeof buf, "%d COMBO", stats_.combo);
        SDL_RenderDebugText(r, centerX(buf, 2.0f), 150, buf);
    }
    SDL_SetRenderScale(r, 1.0f, 1.0f);

    // 直前の判定: 判定ごとの色で表示する。
    //   通常は 2 倍サイズ。PERFECT だけは拡大するアニメーションで強調する
    //   MISS は左右に震えながら少し沈む
    const double judgeAge = t - lastJudgeTime_;
    if (!lastJudgeText_.empty() && judgeAge >= 0 && judgeAge < 500) {
        const JudgeStyle& s = styleOf(lastJudge_);
        const float age = (float)judgeAge;
        const float scale = (lastJudge_ == Judge::Perfect) ? perfectTextScale(age) : 2.0f;

        // 文字の中心を画面上の (中央 + dx, 348 + dy) px に置く（2 倍表示で y = 170 の位置）
        float dx = 0.0f, dy = 0.0f;
        if (lastJudge_ == Judge::Miss) {
            if (age < kMissShakeMs) dx = std::sin(age * 0.09f) * 6.0f * (1.0f - age / (float)kMissShakeMs);
            dy = std::min(age, 200.0f) / 200.0f * 10.0f;
        }
        SDL_SetRenderDrawColor(r, s.r, s.g, s.b, 255);
        SDL_SetRenderScale(r, scale, scale);
        SDL_RenderDebugText(r, centerX(lastJudgeText_.c_str(), scale) + dx / scale, (348.0f + dy) / scale - 4.0f,
                            lastJudgeText_.c_str());
        SDL_SetRenderScale(r, 1.0f, 1.0f);
    }

    // 等倍サイズ: 画面下のオフセット・オートプレイ表示・経過時間・各レーンのキー名
    SDL_SetRenderDrawColor(r, 170, 170, 190, 255);
    
    std::snprintf(buf, sizeof buf, "OFFSET %+.0fms (UP/DOWN)  %s", offsetMs_, autoplay_ ? "AUTO" : "");
    
    SDL_RenderDebugText(r, 8, kScreenH - 40, buf);
    
    std::snprintf(buf, sizeof buf, "TIME %.2fs", t / 1000.0);
    
    SDL_RenderDebugText(r, 8, kScreenH - 24, buf);
    for (int lane = 0; lane < kLanes; ++lane) {
        const char* label = SDL_GetKeyName(kLaneKeys[lane]);
        SDL_RenderDebugText(r, kFieldX + lane * kLaneW + kLaneW / 2 - 4, kJudgeY + 20, label);
    }

    // 曲が終わっていればリザルトを上に重ねる
    if (finished_) renderResult(r);
}

void Game::renderResult(SDL_Renderer* r) {
    // 半透明の黒い枠
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 190);
    
    SDL_FRect bg{40, 240, kScreenW - 80.0f, 300};
    
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // 見出しと、判定ごとの回数・最大コンボ・スコアの一覧
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(r, 255, 220, 120, 255);
    SDL_RenderDebugText(r, centerX("RESULT", 2.0f), 135, "RESULT");
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    
    char buf[64];
    const int rows[] = {stats_.perfect, stats_.great, stats_.good, stats_.miss, stats_.maxCombo};
    const char* names[] = {"PERFECT", "GREAT", "GOOD", "MISS", "MAX COMBO"};
    for (int i = 0; i < 5; ++i) {
        std::snprintf(buf, sizeof buf, "%-10s %4d", names[i], rows[i]);
        SDL_RenderDebugText(r, 50, 160 + i * 14, buf);
    }
    std::snprintf(buf, sizeof buf, "SCORE %07ld", stats_.score);
    
    SDL_RenderDebugText(r, 50, 236, buf);
    SDL_SetRenderScale(r, 1.0f, 1.0f);

    // 操作案内
    SDL_RenderDebugText(r, centerX("R: RETRY   ESC: QUIT", 1.0f), 510, "R: RETRY   ESC: QUIT");
}