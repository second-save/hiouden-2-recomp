#include "cpu/cpu386.h"
#include "hw/audio.h"
#include "hw/dos_runtime.h"
#include "hw/ega_vga.h"

#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

uint8_t sdl_to_scancode(SDL_Keycode key) {
    switch (key) {
        case SDLK_ESCAPE: return 0x01;
        case SDLK_1: return 0x02;
        case SDLK_2: return 0x03;
        case SDLK_3: return 0x04;
        case SDLK_4: return 0x05;
        case SDLK_5: return 0x06;
        case SDLK_6: return 0x07;
        case SDLK_7: return 0x08;
        case SDLK_8: return 0x09;
        case SDLK_9: return 0x0A;
        case SDLK_0: return 0x0B;
        case SDLK_BACKSPACE: return 0x0E;
        case SDLK_TAB: return 0x0F;
        case SDLK_q: return 0x10;
        case SDLK_w: return 0x11;
        case SDLK_e: return 0x12;
        case SDLK_r: return 0x13;
        case SDLK_t: return 0x14;
        case SDLK_y: return 0x15;
        case SDLK_u: return 0x16;
        case SDLK_i: return 0x17;
        case SDLK_o: return 0x18;
        case SDLK_p: return 0x19;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return 0x1C;
        case SDLK_LCTRL:
        case SDLK_RCTRL: return 0x1D;
        case SDLK_a: return 0x1E;
        case SDLK_s: return 0x1F;
        case SDLK_d: return 0x20;
        case SDLK_f: return 0x21;
        case SDLK_g: return 0x22;
        case SDLK_h: return 0x23;
        case SDLK_j: return 0x24;
        case SDLK_k: return 0x25;
        case SDLK_l: return 0x26;
        case SDLK_LSHIFT: return 0x2A;
        case SDLK_z: return 0x2C;
        case SDLK_x: return 0x2D;
        case SDLK_c: return 0x2E;
        case SDLK_v: return 0x2F;
        case SDLK_b: return 0x30;
        case SDLK_n: return 0x31;
        case SDLK_m: return 0x32;
        case SDLK_RSHIFT: return 0x36;
        case SDLK_LALT:
        case SDLK_RALT: return 0x38;
        case SDLK_SPACE: return 0x39;
        case SDLK_F1: return 0x3B;
        case SDLK_F2: return 0x3C;
        case SDLK_F3: return 0x3D;
        case SDLK_F4: return 0x3E;
        case SDLK_F5: return 0x3F;
        case SDLK_F6: return 0x40;
        case SDLK_F7: return 0x41;
        case SDLK_F8: return 0x42;
        case SDLK_F9: return 0x43;
        case SDLK_F10: return 0x44;
        case SDLK_UP: return 0x48;
        case SDLK_PAGEUP: return 0x49;
        case SDLK_LEFT: return 0x4B;
        case SDLK_RIGHT: return 0x4D;
        case SDLK_DOWN: return 0x50;
        case SDLK_PAGEDOWN: return 0x51;
        default: return 0;
    }
}

uint16_t sdl_to_bios_key(SDL_Keycode key, uint8_t scancode) {
    if (key >= SDLK_a && key <= SDLK_z) {
        return static_cast<uint16_t>((scancode << 8) | static_cast<uint8_t>(key));
    }
    if (key >= SDLK_0 && key <= SDLK_9) {
        return static_cast<uint16_t>((scancode << 8) | static_cast<uint8_t>(key));
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) return static_cast<uint16_t>((scancode << 8) | '\r');
    if (key == SDLK_ESCAPE) return static_cast<uint16_t>((scancode << 8) | 0x1B);
    if (key == SDLK_SPACE) return static_cast<uint16_t>((scancode << 8) | ' ');
    return static_cast<uint16_t>(scancode << 8);
}

struct TestClick {
    int x = 0;
    int y = 0;
    int frame = 0;
};

void print_usage(const char* argv0) {
    std::fprintf(stderr,
                 "Usage: %s [options]\n"
                 "  --data-dir <path>            Game data directory (default: hio2)\n"
                 "  --scale <N>                  Window integer scale factor (default: 2)\n"
                 "  --headless-test <frames>     Run N frames headlessly and exit\n"
                 "  --dump-frame <path.bmp>      Save final 640x400 framebuffer to BMP\n"
                 "  --test-click <x>,<y>,<frame> Inject mouse move + left click in test mode\n"
                 "  --trace-overlays             Log INT 8Eh overlays and INT B5h/21h file I/O\n",
                 argv0);
}

} // namespace

int main(int argc, char** argv) {
    std::string data_dir = "hio2";
    int scale = 2;
    int headless_frames = -1;
    std::string dump_frame_path;
    bool trace_overlays = false;
    std::vector<TestClick> test_clicks;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (arg == "--scale" && i + 1 < argc) {
            scale = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--headless-test" && i + 1 < argc) {
            headless_frames = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--dump-frame" && i + 1 < argc) {
            dump_frame_path = argv[++i];
        } else if (arg == "--test-click" && i + 1 < argc) {
            TestClick tc;
            if (std::sscanf(argv[++i], "%d,%d,%d", &tc.x, &tc.y, &tc.frame) == 3) {
                test_clicks.push_back(tc);
            } else {
                std::fprintf(stderr, "Invalid --test-click format (expected x,y,frame)\n");
                return 1;
            }
        } else if (arg == "--trace-overlays") {
            trace_overlays = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", arg.c_str());
            print_usage(argv[0]);
            return 1;
        }
    }

    const bool headless = (headless_frames > 0);

    EgaVga vga;
    Cpu386 cpu(&vga);
    AudioEngine audio;
    DosRuntime dos(&cpu, &vga, &audio);

    cpu.set_int_handler([&](uint8_t vec) { return dos.handle_int(vec); });
    cpu.set_port_handlers(
        [&](uint16_t port, int size) { return dos.port_in(port, size); },
        [&](uint16_t port, uint16_t val, int size) { dos.port_out(port, val, size); }
    );

    if (!audio.init(headless)) {
        std::fprintf(stderr, "[Main] Warning: audio initialization failed, continuing muted.\n");
    }

    if (!dos.boot(data_dir, trace_overlays)) {
        std::fprintf(stderr, "[Main] Failed to boot HIOSYS.EXE from '%s'\n", data_dir.c_str());
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    if (!headless) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            std::fprintf(stderr, "[Main] SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
            return 1;
        }
        window = SDL_CreateWindow(
            "Hiouden II (macOS Native Runtime)",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            640 * scale, 400 * scale,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        if (!window) {
            std::fprintf(stderr, "[Main] SDL_CreateWindow failed: %s\n", SDL_GetError());
            return 1;
        }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        }
        SDL_RenderSetLogicalSize(renderer, 640, 400);
        SDL_ShowCursor(SDL_DISABLE);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 400);
    }

    std::vector<uint32_t> framebuffer(640 * 400, 0xFF000000u);
    bool running = true;
    int frame = 0;

    int mouse_x = 320;
    int mouse_y = 200;
    uint16_t mouse_btn = 0;

    const auto frame_duration = std::chrono::microseconds(16667);

    while (running && !cpu.halted()) {
        const auto frame_start = std::chrono::steady_clock::now();

        if (!headless) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) {
                    running = false;
                } else if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
                    const uint8_t sc = sdl_to_scancode(ev.key.keysym.sym);
                    if (sc != 0) {
                        const uint16_t bk = sdl_to_bios_key(ev.key.keysym.sym, sc);
                        dos.push_key_scancode(sc, bk, ev.type == SDL_KEYDOWN);
                    }
                } else if (ev.type == SDL_MOUSEMOTION) {
                    mouse_x = ev.motion.x;
                    mouse_y = ev.motion.y;
                    dos.set_mouse_state(mouse_x, mouse_y, mouse_btn);
                } else if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
                    const bool down = (ev.type == SDL_MOUSEBUTTONDOWN);
                    uint16_t bit = 0;
                    if (ev.button.button == SDL_BUTTON_LEFT) bit = 1;
                    else if (ev.button.button == SDL_BUTTON_RIGHT) bit = 2;
                    if (down) mouse_btn |= bit;
                    else mouse_btn &= ~bit;
                    mouse_x = ev.button.x;
                    mouse_y = ev.button.y;
                    dos.set_mouse_state(mouse_x, mouse_y, mouse_btn);
                }
            }
            int kbd_guard = 0;
            while (dos.has_pending_scancode() && cpu.get_flag(Cpu386::FLAG_IF) && kbd_guard++ < 16) {
                cpu.trigger_irq(0x09);
            }
        } else {
            for (const auto& tc : test_clicks) {
                if (frame == tc.frame - 2) {
                    dos.set_mouse_state(tc.x, tc.y, 0);
                } else if (frame == tc.frame) {
                    dos.set_mouse_state(tc.x, tc.y, 1);
                } else if (frame == tc.frame + 2) {
                    dos.set_mouse_state(tc.x, tc.y, 0);
                }
            }
        }

        dos.dispatch_mouse_callback();

        vga.begin_frame();
        constexpr int kMaxInsnsPerFrame = 120000;
        int vblank_polls = 0;
        int timer_reads = 0;
        bool vblank_triggered = false;

        for (int i = 0; i < kMaxInsnsPerFrame && !cpu.halted(); ++i) {
            cpu.clear_vblank_poll();
            cpu.clear_timer_read();

            if (!cpu.step()) break;

            if (cpu.hit_vblank_poll()) {
                ++vblank_polls;
                if (!vblank_triggered && vblank_polls >= 4) {
                    vga.enter_vblank();
                    vblank_triggered = true;
                } else if (vblank_triggered && !vga.in_vblank() && vblank_polls >= 8) {
                    // Guest finished this VBlank iteration and began polling for the next frame
                    break;
                }
            }

            if (cpu.hit_timer_read()) {
                ++timer_reads;
                if (timer_reads % 16 == 0) {
                    cpu.trigger_irq(0x08);
                }
            }

            if (i == kMaxInsnsPerFrame - 5000 && !vblank_triggered) {
                vga.enter_vblank();
                vblank_triggered = true;
            }
        }

        if ((frame % 3) == 0 && !cpu.halted()) {
            cpu.trigger_irq(0x08);
        }

        vga.render_argb(framebuffer.data());

        if (!headless) {
            SDL_UpdateTexture(texture, nullptr, framebuffer.data(), 640 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            const auto elapsed = std::chrono::steady_clock::now() - frame_start;
            if (elapsed < frame_duration) {
                SDL_Delay(static_cast<Uint32>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(frame_duration - elapsed).count()));
            }
        } else {
            if (frame % 100 == 0 || frame == headless_frames - 1) {
                int non_black = 0;
                for (uint32_t px : framebuffer) {
                    if ((px & 0x00FFFFFFu) != 0) ++non_black;
                }
                std::fprintf(stderr,
                             "[Headless] frame=%d insns=%llu vram_writes=%llu non_black=%d/%d CS:IP=%04X:%04X\n",
                             frame,
                             static_cast<unsigned long long>(cpu.insn_count()),
                             static_cast<unsigned long long>(vga.total_vram_writes()),
                             non_black, 640 * 400,
                             cpu.sreg[Cpu386::SEG_CS], cpu.ip);
            }
        }

        ++frame;
        if (headless && frame >= headless_frames) {
            running = false;
        }
    }

    if (!dump_frame_path.empty()) {
        if (vga.save_bmp(dump_frame_path)) {
            std::fprintf(stderr, "[Main] Saved frame %d to %s\n", frame, dump_frame_path.c_str());
        } else {
            std::fprintf(stderr, "[Main] Failed to save frame to %s\n", dump_frame_path.c_str());
        }
    }

    if (!headless) {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
    audio.shutdown();

    return cpu.error() ? 2 : 0;
}
