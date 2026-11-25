#include "cpu/cpu386.h"
#include "hw/audio.h"
#include "hw/dos_runtime.h"
#include "hw/ega_vga.h"
#include "installer.h"
#include "launcher.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
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
        case SDLK_Q: return 0x10;
        case SDLK_W: return 0x11;
        case SDLK_E: return 0x12;
        case SDLK_R: return 0x13;
        case SDLK_T: return 0x14;
        case SDLK_Y: return 0x15;
        case SDLK_U: return 0x16;
        case SDLK_I: return 0x17;
        case SDLK_O: return 0x18;
        case SDLK_P: return 0x19;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return 0x1C;
        case SDLK_LCTRL:
        case SDLK_RCTRL: return 0x1D;
        case SDLK_A: return 0x1E;
        case SDLK_S: return 0x1F;
        case SDLK_D: return 0x20;
        case SDLK_F: return 0x21;
        case SDLK_G: return 0x22;
        case SDLK_H: return 0x23;
        case SDLK_J: return 0x24;
        case SDLK_K: return 0x25;
        case SDLK_L: return 0x26;
        case SDLK_LSHIFT: return 0x2A;
        case SDLK_Z: return 0x2C;
        case SDLK_X: return 0x2D;
        case SDLK_C: return 0x2E;
        case SDLK_V: return 0x2F;
        case SDLK_B: return 0x30;
        case SDLK_N: return 0x31;
        case SDLK_M: return 0x32;
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
    if (key >= SDLK_A && key <= SDLK_Z) {  // SDL3 letter keycodes are the lowercase ASCII values
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
    int buttons = 1; // 1 = left, 2 = right
};

void print_usage(const char* argv0) {
    std::fprintf(stderr,
                 "Usage: %s [options]\n"
                 "  --data-dir <path>            Game data directory (no flag: the launcher installs and picks it)\n"
                 "  --fullscreen                 Start fullscreen (Alt+Enter toggles)\n"
                 "  --install <dir>              Install + patch your game files from <dir>, then exit\n"
                 "  --scale <N>                  Window integer scale factor (default: 2)\n"
                 "  --headless-test <frames>     Run N frames headlessly and exit\n"
                 "  --dump-frame <path.bmp>      Save final 640x400 framebuffer to BMP\n"
                 "  --dump-every <N> <dir>       Save every Nth frame as <dir>/fNNNNNN.bmp (dir must exist)\n"
                 "  --test-click <x>,<y>,<frame>[,<btn>] Inject a click in test mode (btn 1=left default, 2=right)\n"
                 "  --remap <FROM>=<TO>          Serve file TO whenever the game opens FROM (debug warps)\n"
                 "  --trace-ip <hexlinear>       Log registers each time CS*16+IP hits this address\n"
                 "  --trace-overlays           Log INT 8Eh overlays and INT B5h/21h file I/O\n"
                 "  --watch <hexlo>-<hexhi>      Log CPU reads/writes to that linear range (first 60, then min/max)\n",
                 argv0);
}

} // namespace

int main(int argc, char** argv) {
    std::string data_dir = "hio2";
    bool data_dir_given = false;
    int scale = 2;
    bool fullscreen = false;
    int headless_frames = -1;
    std::string dump_frame_path;
    std::string dump_every_dir;
    int dump_every = 0;
    bool trace_overlays = false;
    std::vector<TestClick> test_clicks;
    std::vector<std::pair<std::string, std::string>> remaps;
    std::vector<uint32_t> trace_ips;
    uint32_t watch_lo = 0, watch_hi = 0;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            data_dir = argv[++i];
            data_dir_given = true;
        } else if (arg == "--scale" && i + 1 < argc) {
            scale = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--fullscreen") {
            fullscreen = true;
        } else if (arg == "--install" && i + 1 < argc) {  // install without the launcher window
            const std::string err = installer::install(argv[++i]);
            std::printf("%s\n", err.empty() ? ("installed into " + installer::game_dir()).c_str() : err.c_str());
            return err.empty() ? 0 : 1;
        } else if (arg == "--headless-test" && i + 1 < argc) {
            headless_frames = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--dump-frame" && i + 1 < argc) {
            dump_frame_path = argv[++i];
        } else if (arg == "--dump-every" && i + 2 < argc) {
            dump_every = std::max(1, std::atoi(argv[++i]));
            dump_every_dir = argv[++i];
        } else if (arg == "--test-click" && i + 1 < argc) {
            TestClick tc;
            if (std::sscanf(argv[++i], "%d,%d,%d,%d", &tc.x, &tc.y, &tc.frame, &tc.buttons) >= 3) {
                test_clicks.push_back(tc);
            } else {
                std::fprintf(stderr, "Invalid --test-click format (expected x,y,frame)\n");
                return 1;
            }
        } else if (arg == "--remap" && i + 1 < argc) {
            std::string spec = argv[++i];
            const size_t eq = spec.find('=');
            if (eq == std::string::npos) {
                std::fprintf(stderr, "Invalid --remap format (expected FROM=TO)\n");
                return 1;
            }
            std::string from = spec.substr(0, eq);
            for (char& c : from) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            remaps.emplace_back(from, spec.substr(eq + 1));
        } else if (arg == "--trace-ip" && i + 1 < argc) {
            trace_ips.push_back(static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 16)));
        } else if (arg == "--trace-overlays") {
            trace_overlays = true;
        } else if (arg == "--watch" && i + 1 < argc) {
            const std::string spec = argv[++i];
            watch_lo = static_cast<uint32_t>(std::strtoul(spec.c_str(), nullptr, 16));
            watch_hi = static_cast<uint32_t>(std::strtoul(spec.substr(spec.find('-') + 1).c_str(), nullptr, 16));
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
    if (!data_dir_given && !headless) {  // players: the launcher installs their game files, then Play
        LaunchOptions lo;
        lo.scale = scale;
        if (!run_launcher(lo)) {
            SDL_Quit();
            return 0;
        }
        data_dir = lo.data_dir;
        scale = lo.scale;
        fullscreen = lo.fullscreen;
    }

    EgaVga vga;
    Cpu386 cpu(&vga);
    cpu.watch_lo = watch_lo;
    cpu.watch_hi = watch_hi;
    AudioEngine audio;
    DosRuntime dos(&cpu, &vga, &audio);
    for (const auto& r : remaps) dos.remap[r.first] = r.second;

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
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            std::fprintf(stderr, "[Main] SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
            return 1;
        }
        window = SDL_CreateWindow("Hiouden II", 640 * scale, 400 * scale,
                                  SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
        if (!window) {
            std::fprintf(stderr, "[Main] SDL_CreateWindow failed: %s\n", SDL_GetError());
            return 1;
        }
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            std::fprintf(stderr, "[Main] SDL_CreateRenderer failed: %s\n", SDL_GetError());
            return 1;
        }
        SDL_SetRenderVSync(renderer, 1);
        SDL_SetRenderLogicalPresentation(renderer, 640, 400, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_HideCursor();
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 400);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
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
                SDL_ConvertEventToRenderCoordinates(renderer, &ev);  // window pixels -> 640x400 game pixels
                if (ev.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_KEY_UP) {
                    if (ev.key.down && ev.key.key == SDLK_RETURN && (ev.key.mod & SDL_KMOD_ALT)) {
                        fullscreen = !fullscreen;  // Alt+Enter toggles fullscreen
                        SDL_SetWindowFullscreen(window, fullscreen);
                        continue;
                    }
                    const uint8_t sc = sdl_to_scancode(ev.key.key);
                    if (sc != 0) {
                        const uint16_t bk = sdl_to_bios_key(ev.key.key, sc);
                        dos.push_key_scancode(sc, bk, ev.type == SDL_EVENT_KEY_DOWN);
                    }
                } else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                    mouse_x = std::clamp(static_cast<int>(ev.motion.x), 0, 639);
                    mouse_y = std::clamp(static_cast<int>(ev.motion.y), 0, 399);
                    dos.set_mouse_state(mouse_x, mouse_y, mouse_btn);
                } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    const bool down = (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
                    uint16_t bit = 0;
                    if (ev.button.button == SDL_BUTTON_LEFT) bit = 1;
                    else if (ev.button.button == SDL_BUTTON_RIGHT) bit = 2;
                    if (down) mouse_btn |= bit;
                    else mouse_btn &= ~bit;
                    mouse_x = std::clamp(static_cast<int>(ev.button.x), 0, 639);
                    mouse_y = std::clamp(static_cast<int>(ev.button.y), 0, 399);
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
                    dos.set_mouse_state(tc.x, tc.y, static_cast<uint16_t>(tc.buttons));
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

            for (uint32_t t : trace_ips) {
                if (((uint32_t(cpu.sreg[Cpu386::SEG_CS]) << 4) + cpu.ip) == t) {
                    std::fprintf(stderr, "[TRACE] f=%d %04X:%04X AX=%04X BX=%04X CX=%04X DX=%04X SI=%04X DI=%04X DS=%04X ES=%04X\n",
                                 frame, cpu.sreg[Cpu386::SEG_CS], cpu.ip, cpu.eax & 0xFFFF, cpu.ebx & 0xFFFF,
                                 cpu.ecx & 0xFFFF, cpu.edx & 0xFFFF, cpu.esi & 0xFFFF, cpu.edi & 0xFFFF,
                                 cpu.sreg[Cpu386::SEG_DS], cpu.sreg[0]);
                    const uint8_t* m = cpu.ram() + (uint32_t(cpu.sreg[Cpu386::SEG_DS]) << 4) + (cpu.edi & 0xFFFF);
                    std::fprintf(stderr, "[TRACE]   DS:DI = %02X %02X %02X %02X %02X %02X %02X %02X\n",
                                 m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7]);
                }
            }
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
        if (dump_every > 0 && frame % dump_every == 0) {
            char name[32];
            std::snprintf(name, sizeof(name), "/f%06d.bmp", frame);
            vga.save_bmp(dump_every_dir + name);
        }

        if (!headless) {
            SDL_UpdateTexture(texture, nullptr, framebuffer.data(), 640 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
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

    if (watch_hi > watch_lo) {
        std::fprintf(stderr, "[WATCH] %u accesses, range %05X-%05X\n", cpu.watch_logged,
                     cpu.watch_logged ? cpu.watch_min : 0, cpu.watch_logged ? cpu.watch_max : 0);
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
