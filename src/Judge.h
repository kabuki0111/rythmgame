// 判定の種類・判定幅・成績（スコア・コンボ）の集計
#pragma once

// 判定幅 (ミリ秒, ±)。ノーツ時刻とのずれがこの範囲内ならその判定になる
constexpr double kPerfectMs = 33.0;
constexpr double kGreatMs = 66.0;
constexpr double kGoodMs = 100.0;

enum class Judge { Perfect, Great, Good, Miss };

// 画面に表示する判定名
const char* judgeName(Judge j);

// 時刻のずれ diffMs (ms, 絶対値) から判定を決める。GOOD の幅を超えたら Miss
Judge judgeByDiff(double diffMs);

// 1 プレイ分の成績
struct JudgeStats {
    int perfect = 0, great = 0, good = 0, miss = 0;  // 判定ごとの回数
    int combo = 0, maxCombo = 0;                     // 現在のコンボと最大コンボ
    long score = 0;

    // 判定 1 回分をカウント・スコア・コンボに反映する
    void add(Judge j);
};
