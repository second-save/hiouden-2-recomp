// Launcher: point it at your own Hiouden II (Taiwanese DOS release), it installs an English copy, then Play.
// Drawn with SDL3's renderer at 640x400 logical with the Spleen 8x16 font (the game's English font).
#include "launcher.h"

#include "font8x16.h"
#include "installer.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>

namespace {

constexpr int W = 640, H = 400;
SDL_Renderer* ren = nullptr;
SDL_Texture* font = nullptr;
float mx = -1, my = -1;
bool clicked = false;

std::mutex pick_mutex;
std::string picked;            // folder chosen in the dialog (set from the dialog callback)
std::atomic<bool> has_pick{false};

void make_font() {
    SDL_Surface* s = SDL_CreateSurface(95 * 8, 16, SDL_PIXELFORMAT_ARGB8888);
    for (int c = 0; c < 95; ++c)
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 8; ++x) {
                const bool on = kFont8x16[c][y] & (0x80 >> x);
                auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(s->pixels) + y * s->pitch);
                row[c * 8 + x] = on ? 0xFFFFFFFFu : 0;
            }
    font = SDL_CreateTextureFromSurface(ren, s);
    SDL_SetTextureScaleMode(font, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(s);
}

int text_w(const std::string& t, int k = 1) { return static_cast<int>(t.size()) * 8 * k; }

void text(const std::string& t, float x, float y, uint32_t rgb, int k = 1) {
    SDL_SetTextureColorMod(font, rgb >> 16 & 255, rgb >> 8 & 255, rgb & 255);
    for (size_t i = 0; i < t.size(); ++i) {
        int c = static_cast<unsigned char>(t[i]);
        if (c < 32 || c > 126) c = '?';
        const SDL_FRect src{static_cast<float>((c - 32) * 8), 0, 8, 16};
        const SDL_FRect dst{x + i * 8.0f * k, y, 8.0f * k, 16.0f * k};
        SDL_RenderTexture(ren, font, &src, &dst);
    }
}

void center(const std::string& t, float y, uint32_t rgb, int k = 1) { text(t, (W - text_w(t, k)) / 2.0f, y, rgb, k); }

void fill(float x, float y, float w, float h, uint32_t rgb) {
    SDL_SetRenderDrawColor(ren, rgb >> 16 & 255, rgb >> 8 & 255, rgb & 255, 255);
    const SDL_FRect r{x, y, w, h};
    SDL_RenderFillRect(ren, &r);
}

void frame(float x, float y, float w, float h, uint32_t rgb) {
    SDL_SetRenderDrawColor(ren, rgb >> 16 & 255, rgb >> 8 & 255, rgb & 255, 255);
    const SDL_FRect r{x, y, w, h};
    SDL_RenderRect(ren, &r);
}

bool hover(float x, float y, float w, float h) { return mx >= x && mx < x + w && my >= y && my < y + h; }

// colours of the game's own windows: dark red panels, gold frames
constexpr uint32_t BG = 0x1a0c0a, PANEL = 0x5a0f0c, GOLD = 0xd8b060, TEXT = 0xf0e6d8, MUTED = 0xa89888,
                   GREEN = 0x8fd18a, RED = 0xff8a7a;

bool button(const std::string& label, float x, float y, float w, bool primary, bool enabled) {
    const bool hv = enabled && hover(x, y, w, 26);
    fill(x, y, w, 26, !enabled ? 0x2a1a16 : primary ? (hv ? 0xe8c070 : 0xc89c48) : (hv ? 0x7a2a22 : PANEL));
    frame(x, y, w, 26, enabled ? GOLD : 0x6a5a4a);
    text(label, x + (w - text_w(label)) / 2, y + 5, !enabled ? MUTED : primary ? 0x200a06 : TEXT);
    return hv && clicked;
}

std::string cfg_path() { return installer::pref_dir() + "launcher.cfg"; }

void load_cfg(LaunchOptions& o) {
    std::ifstream f(cfg_path());
    std::string k;
    int v;
    while (f >> k >> v) {
        if (k == "scale") o.scale = SDL_clamp(v, 1, 4);
        if (k == "fullscreen") o.fullscreen = v != 0;
    }
}

void save_cfg(const LaunchOptions& o) {
    std::ofstream f(cfg_path());
    f << "scale " << o.scale << "\nfullscreen " << (o.fullscreen ? 1 : 0) << "\n";
}

void SDLCALL on_pick(void*, const char* const* list, int) {
    if (!list || !list[0]) return;  // cancelled or failed
    std::lock_guard<std::mutex> lock(pick_mutex);
    picked = list[0];
    has_pick = true;
}

}  // namespace

bool run_launcher(LaunchOptions& out) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return false;
    load_cfg(out);
    SDL_Window* win = nullptr;
    if (!SDL_CreateWindowAndRenderer("Hiouden II", W * 2, H * 2, SDL_WINDOW_HIGH_PIXEL_DENSITY, &win, &ren)) {
        std::fprintf(stderr, "[Launcher] %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderLogicalPresentation(ren, W, H, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(ren, 1);
    make_font();

    bool play = false, quit = false, installing = false;
    std::string pending, message;
    uint32_t message_rgb = TEXT;
    while (!play && !quit) {
        clicked = false;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            SDL_ConvertEventToRenderCoordinates(ren, &ev);
            if (ev.type == SDL_EVENT_QUIT) quit = true;
            if (ev.type == SDL_EVENT_MOUSE_MOTION) { mx = ev.motion.x; my = ev.motion.y; }
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && ev.button.button == SDL_BUTTON_LEFT) {
                mx = ev.button.x; my = ev.button.y; clicked = true;
            }
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE) quit = true;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_RETURN && installer::is_installed()) play = true;
        }
        if (has_pick) {
            std::lock_guard<std::mutex> lock(pick_mutex);
            pending = picked;
            has_pick = false;
            installing = true;
            message = "Installing...";
            message_rgb = TEXT;
        } else if (installing) {  // one frame after "Installing..." was drawn
            const std::string err = installer::install(pending);
            installing = false;
            message = err.empty() ? "Installed. The English patch is applied." : err;
            message_rgb = err.empty() ? GREEN : RED;
        }

        const bool ok = installer::is_installed();
        fill(0, 0, W, H, BG);
        center("HIOUDEN II", 34, GOLD, 3);
        center("English translation", 92, TEXT);

        fill(60, 128, 520, 96, PANEL);
        frame(60, 128, 520, 96, GOLD);
        text("Game files", 76, 140, GOLD);
        if (ok) {
            text("Installed, English patch applied.", 76, 162, GREEN);
            text("Choose the folder again to reinstall (saves are kept).", 76, 184, MUTED);
        } else if (installer::needs_update()) {
            text("New version of the English patch. Choose your game", 76, 162, GOLD);
            text("folder again to update (saves are kept).", 76, 184, GOLD);
        } else {
            text("Not installed. Choose the folder of your own copy of", 76, 162, TEXT);
            text("Hiouden II, the Taiwanese DOS release (HIO.EXE).", 76, 184, TEXT);
        }
        if (button("Choose game folder...", 76, 236, 220, false, !installing))
            SDL_ShowOpenFolderDialog(on_pick, nullptr, win, nullptr, false);

        text("Window size", 76, 280, MUTED);
        const std::string sz = std::to_string(640 * out.scale) + " x " + std::to_string(400 * out.scale);
        if (button(sz, 220, 276, 140, false, !out.fullscreen)) out.scale = out.scale % 4 + 1;
        text("Display", 76, 312, MUTED);
        if (button(out.fullscreen ? "Fullscreen" : "Window", 220, 308, 140, false, true)) out.fullscreen = !out.fullscreen;

        if (button("Play", 444, 236, 120, true, ok && !installing)) play = true;
        if (button("Quit", 444, 276, 120, false, true)) quit = true;

        text(message, 76, 346, message_rgb);
        center("Second Save  -  Alt+Enter: fullscreen in game", 376, MUTED);
        if (const char* shot = SDL_getenv("HIOUDEN_LAUNCHER_SHOT")) {  // docs/tests: save this frame and quit
            if (SDL_Surface* s = SDL_RenderReadPixels(ren, nullptr)) {
                SDL_SaveBMP(s, shot);
                SDL_DestroySurface(s);
            }
            quit = true;
        }
        SDL_RenderPresent(ren);
    }
    save_cfg(out);
    out.data_dir = installer::game_dir();
    SDL_DestroyTexture(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    ren = nullptr;
    return play;
}
