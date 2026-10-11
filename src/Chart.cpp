// 譜面ファイル (chart.txt) の読み込み
#include "Chart.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "Config.h"

std::vector<Note> loadChart(const std::string& path) {
    std::vector<Note> notes;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;  // 空行・コメント行は飛ばす
        // 「時刻 レーン」を読み取り、レーン番号が範囲内のものだけ採用する
        std::istringstream ss(line);
        Note n{};
        if (!(ss >> n.timeMs >> n.lane) || n.lane < 0 || n.lane >= kLanes) continue;
        // 3 列目（ロングノーツの終点）は省略可。始点以前の値なら通常ノーツとして扱う
        if (!(ss >> n.endMs) || n.endMs <= n.timeMs) n.endMs = 0.0;
        notes.push_back(n);
    }
    // 判定と描画は「先頭から順に見れば時刻順」を前提にしているので必ずソートする
    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) { return a.timeMs < b.timeMs; });
    return notes;
}
