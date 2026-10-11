// 曲の再生と、再生位置を基準にしたゲーム内時計
#pragma once

#include <SDL3/SDL.h>

// ---------------------------------------------------------------------------
// オーディオの再生位置を「時計」として使うクラス。
// 送り込んだバイト数 − ストリームに残っているバイト数 = デバイスに渡った量。
// この値はデバイスが塊で取り出すたびに段階的に進むので、間はOSの時計で補間する。
// ---------------------------------------------------------------------------
class AudioClock {
public:
    // WAV を読み込み、既定の再生デバイスを開く。失敗したら false
    bool open(const char* wavPath);

    // 曲を先頭から再生し直し、時計を 0 ms に戻す
    void start();

    // 毎フレーム呼ぶ。再生位置が進んでいたら基準点を更新する。
    void update();

    // 任意の SDL タイムスタンプ (ns) を曲の時刻 (ms) に変換する
    double timeAt(Uint64 ticksNs) const;

    double now() const { return timeAt(SDL_GetTicksNS()); }  // 現在の曲の時刻 (ms)
    double durationMs() const { return durationMs_; }        // 曲の長さ (ms)

    // 再生デバイスと WAV データを解放する
    void close();

private:
    SDL_AudioSpec spec_{};              // WAV のフォーマット（周波数・チャンネル数など）
    Uint8* data_ = nullptr;             // WAV の PCM データ
    Uint32 len_ = 0;                    // PCM データのバイト数
    SDL_AudioStream* stream_ = nullptr; // 再生デバイスにつながったストリーム
    int frameSize_ = 1;                 // 1 サンプルフレームのバイト数
    double durationMs_ = 0.0;           // 曲の長さ (ms)
    double lastPosMs_ = 0.0;            // 最後に観測した再生位置 (ms)
    Uint64 lastTickNs_ = 0;             // その再生位置を観測した時刻 (ns)
};
