// 最小の 4 レーン落下型リズムゲーム (SDL3)
//   操作: D F J K = 各レーン / ↑↓ = 判定オフセット調整 / A = オートプレイ
//         R = リスタート / Esc = 終了
//
// このファイルはエントリポイント。SDL の初期化とウィンドウ作成を行い、
// 「入力 → 更新 → 描画」のメインループを回す。ゲームの中身は Game クラスにある。
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <string>

#include "Config.h"
#include "Game.h"

int main(int argc, char* argv[]) {
    // 開発用: --shot <ms> でその時刻のスクリーンショットを shot.bmp に保存して終了
    double shotAtMs = -1.0;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--shot") shotAtMs = std::atof(argv[i + 1]);

    // SDL の初期化（映像と音声）
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init 失敗: %s", SDL_GetError());
        return 1;
    }

    // ウィンドウとレンダラーの作成。論理解像度を固定し、拡大縮小時は余白付きで表示する
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Rhythm Game", kScreenW, kScreenH, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("ウィンドウ作成失敗: %s", SDL_GetError());
        return 1;
    }
    SDL_SetRenderLogicalPresentation(renderer, kScreenW, kScreenH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(renderer, 1);

    // 実行ファイルの隣の assets/ から曲と譜面を読み込んで開始
    const std::string assetDir = std::string(SDL_GetBasePath()) + "assets/";
    Game game;
    if (!game.init(assetDir)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Rhythm Game", "assets/song.wav と assets/chart.txt を確認してください", window);
        return 1;
    }
    if (shotAtMs >= 0) game.setAutoplay(true);

    // メインループ
    bool running = true;
    while (running) {
        // 入力: 溜まっているイベントをすべて処理する
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
            else if (e.type == SDL_EVENT_KEY_DOWN) game.onKeyDown(e.key);
            else if (e.type == SDL_EVENT_KEY_UP) game.onKeyUp(e.key);
        }

        // 更新と描画
        game.update();
        game.render(renderer);

        // --shot 指定時: 指定時刻に達したら画面を保存して終了
        if (shotAtMs >= 0 && game.time() >= shotAtMs) {
            if (SDL_Surface* s = SDL_RenderReadPixels(renderer, nullptr)) {
                SDL_SaveBMP(s, "shot.bmp");
                SDL_DestroySurface(s);
            }
            running = false;
        }
        SDL_RenderPresent(renderer);
        if (shotAtMs >= 0) SDL_Delay(1);
    }

    // 後片付け
    game.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
