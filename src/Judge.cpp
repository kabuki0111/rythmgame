// 判定名の変換と、判定結果をスコア・コンボに反映する処理
#include "Judge.h"

#include <algorithm>

const char* judgeName(Judge j) {
    switch (j) {
    case Judge::Perfect: return "PERFECT";
    case Judge::Great: return "GREAT";
    case Judge::Good: return "GOOD";
    case Judge::Miss: return "MISS";
    }
    return "";
}

Judge judgeByDiff(double diffMs) {
    if (diffMs <= kPerfectMs) return Judge::Perfect;
    if (diffMs <= kGreatMs) return Judge::Great;
    if (diffMs <= kGoodMs) return Judge::Good;
    return Judge::Miss;
}

void JudgeStats::add(Judge j) {
    // 判定ごとの回数とスコア加算（PERFECT 1000 / GREAT 700 / GOOD 300 / MISS 0）
    switch (j) {
    case Judge::Perfect: ++perfect; score += 1000; break;
    case Judge::Great: ++great; score += 700; break;
    case Judge::Good: ++good; score += 300; break;
    case Judge::Miss: ++miss; break;
    }
    // MISS でコンボが途切れる。それ以外はコンボを伸ばし最大コンボを更新する
    if (j == Judge::Miss) {
        combo = 0;
    } else {
        maxCombo = std::max(maxCombo, ++combo);
    }
}
