"""サンプル曲 (assets/song.wav) と譜面 (assets/chart.txt) を生成するスクリプト。
自作の曲を使う場合は不要。  使い方: python tools/make_sample.py
"""
import os
import random
import wave

import numpy as np

SR = 48000
BPM = 140
BARS = 16
INTRO_BEATS = 4          # 最初の 1 小節はノーツなし（カウント）
BEAT = 60.0 / BPM

out_dir = os.path.join(os.path.dirname(__file__), "..", "assets")
total_beats = INTRO_BEATS + BARS * 4
length = int(SR * (total_beats * BEAT + 1.5))
buf = np.zeros(length, dtype=np.float64)


def add(sig, t):
    i = int(t * SR)
    n = min(len(sig), length - i)
    if n > 0:
        buf[i:i + n] += sig[:n]


def kick():
    t = np.arange(int(0.25 * SR)) / SR
    freq = 50 + 110 * np.exp(-t * 30)
    return 0.9 * np.sin(2 * np.pi * np.cumsum(freq) / SR) * np.exp(-t * 9)


def hat(rng):
    t = np.arange(int(0.05 * SR)) / SR
    noise = rng.uniform(-1, 1, len(t))
    noise = np.diff(noise, prepend=0)  # 簡易ハイパス
    return 0.18 * noise * np.exp(-t * 70)


def click(high):
    t = np.arange(int(0.06 * SR)) / SR
    f = 1760 if high else 880
    return 0.4 * np.sin(2 * np.pi * f * t) * np.exp(-t * 60)


def bass(freq, dur):
    t = np.arange(int(dur * SR)) / SR
    env = np.minimum(1, t * 200) * np.exp(-t * 4)
    return 0.3 * np.sign(np.sin(2 * np.pi * freq * t)) * env * 0.5


rng = np.random.default_rng(1)
# イントロのカウント
for b in range(INTRO_BEATS):
    add(click(b == 0), b * BEAT)

roots = [55.0, 55.0, 43.65, 49.0]  # A, A, F, G
for bar in range(BARS):
    for beat in range(4):
        t = (INTRO_BEATS + bar * 4 + beat) * BEAT
        add(kick(), t)
        add(hat(rng), t + BEAT / 2)
        add(hat(rng), t)
        add(bass(roots[(bar // 2) % 4], BEAT * 0.45), t + BEAT / 2)

buf /= max(1.0, np.max(np.abs(buf)))
pcm = (buf * 32767 * 0.9).astype("<i2")
with wave.open(os.path.join(out_dir, "song.wav"), "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(SR)
    w.writeframes(pcm.tobytes())

# 譜面: 8 分音符グリッド上にパターンを置く
# 1 行 = 時刻(ms) レーン [終点の時刻(ms)]  ※ 3 列目があればロングノーツ
LONG_BARS = range(4, 12)   # ロングノーツを置く小節（中盤）
LONG_RATE = 0.35           # 拍頭でロングノーツにする確率
LONG_BEATS = [1, 2]        # ロングノーツの長さ（拍）

random.seed(7)
lines = ["# 譜面ファイル: 1行 = 時刻(ミリ秒) レーン(0-3) [終点の時刻(ミリ秒)]",
         f"# サンプル曲: {BPM} BPM, 最初の {INTRO_BEATS} 拍はカウント"]


def ms(beat_pos):
    return f"{beat_pos * BEAT * 1000:.1f}"


prev = -1
busy_until = [0.0] * 4     # 各レーンがロングノーツで塞がっている終点の拍
for bar in range(BARS):
    density = 1 if bar < 4 else 2          # 前半は 4 分、後半は 8 分
    for step in range(4 * density):
        if density == 2 and step % 2 == 1 and random.random() < 0.4:
            continue
        beat_pos = INTRO_BEATS + bar * 4 + step / density
        # 押し続けているレーンには置かない
        free = [l for l in range(4) if busy_until[l] < beat_pos]
        lane = random.choice([l for l in free if l != prev])
        prev = lane

        # 中盤の拍頭はロングノーツにする（同時に押し続けるのは 1 本まで）
        holding = any(b >= beat_pos for b in busy_until)
        if bar in LONG_BARS and step % density == 0 and not holding and random.random() < LONG_RATE:
            end = beat_pos + random.choice(LONG_BEATS)
            busy_until[lane] = end
            lines.append(f"{ms(beat_pos)} {lane} {ms(end)}")
        else:
            lines.append(f"{ms(beat_pos)} {lane}")

        if bar >= 12 and step % (2 * density) == 0:   # 終盤は同時押し
            other = random.choice([l for l in free if l != lane])
            lines.append(f"{ms(beat_pos)} {other}")

with open(os.path.join(out_dir, "chart.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
long_count = sum(1 for l in lines[2:] if len(l.split()) == 3)
print("generated", len(lines) - 2, "notes (long:", long_count, ")")
