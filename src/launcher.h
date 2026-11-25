#pragma once
#include <string>

struct LaunchOptions {
    std::string data_dir;
    int scale = 2;
    bool fullscreen = false;
};

// Shows the launcher (install the player's game files, settings, Play). Returns false if the player quit.
bool run_launcher(LaunchOptions& out);
