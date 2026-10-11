// ノーツの定義と譜面ファイルの読み込み
#pragma once

#include <string>
#include <vector>

// 譜面上の 1 ノーツ
struct Note {
    double timeMs;         // 叩くべき曲の時刻 (ms)。ロングノーツでは始点
    int lane;              // レーン番号 (0〜3)
    double endMs = 0.0;    // ロングノーツの終点の時刻 (ms)。通常ノーツは 0
    bool holding = false;  // ロングノーツの始点を叩いて押し続けている最中なら true
    bool done = false;     // 判定済み（叩いた・見逃した・離した）なら true

    // 終点が始点より後ならロングノーツ
    bool isLong() const { return endMs > timeMs; }
};

// 譜面ファイルを読み込み、時刻順にソートして返す。
// 1 行 = `時刻(ms) レーン(0〜3) [終点の時刻(ms)]`、空行と `#` 始まりの行は無視する。
// 3 列目があればロングノーツ、なければ通常ノーツ。
std::vector<Note> loadChart(const std::string& path);
