// AudioClock の実装: WAV の読み込み・再生と、再生位置から曲の時刻を求める処理
#include "AudioClock.h"

#include <algorithm>

bool AudioClock::open(const char* wavPath) {
    // WAV ファイルを丸ごとメモリに読み込む
    if (!SDL_LoadWAV(wavPath, &spec_, &data_, &len_)) {
        SDL_Log("WAV 読み込み失敗: %s", SDL_GetError());
        return false;
    }
    // WAV と同じフォーマットで既定の再生デバイスを開く（この時点ではまだ一時停止）
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec_, nullptr, nullptr);
    if (!stream_) {
        SDL_Log("オーディオデバイスを開けません: %s", SDL_GetError());
        return false;
    }
    // 曲の長さ = 総フレーム数 ÷ サンプリング周波数
    frameSize_ = SDL_AUDIO_FRAMESIZE(spec_);
    durationMs_ = (double)len_ / frameSize_ * 1000.0 / spec_.freq;
    return true;
}

void AudioClock::start() {
    // 残っている音声を捨て、曲全体を一度にストリームへ送り込む
    SDL_ClearAudioStream(stream_);
    SDL_PutAudioStreamData(stream_, data_, (int)len_);
    SDL_FlushAudioStream(stream_);
    // 時計の基準点を「今 = 0 ms」にしてから再生を開始する
    lastPosMs_ = 0.0;
    lastTickNs_ = SDL_GetTicksNS();
    SDL_ResumeAudioStreamDevice(stream_);
}

void AudioClock::update() {
    // 再生済みフレーム数 = 送り込んだ量 − ストリームに残っている量
    const int queued = SDL_GetAudioStreamQueued(stream_);
    const double played = (double)(len_ - (Uint32)std::max(queued, 0)) / frameSize_;
    const double posMs = played * 1000.0 / spec_.freq;
    // 再生位置が進んだときだけ基準点（位置と観測時刻）を更新する
    if (posMs != lastPosMs_) {
        lastPosMs_ = posMs;
        lastTickNs_ = SDL_GetTicksNS();
    }
}

double AudioClock::timeAt(Uint64 ticksNs) const {
    // 基準点からの経過時間を足して、段階的な再生位置の間を補間する
    const double delta = ((double)ticksNs - (double)lastTickNs_) / 1.0e6;
    return lastPosMs_ + delta;
}

void AudioClock::close() {
    if (stream_) SDL_DestroyAudioStream(stream_);
    if (data_) SDL_free(data_);
}
