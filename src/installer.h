#pragma once
// Installer: copy the player's own Hiouden II (Taiwanese DOS release) into the app data folder and apply the
// English patches (patches/*.bps shipped next to the program; they hold no game data). The player's folder is
// only read, never written.
#include <string>

namespace installer {

std::string pref_dir();     // per-user app folder, e.g. ~/Library/Application Support/Second Save/Hiouden II/
std::string game_dir();     // pref_dir() + "game/": the patched copy the game runs from
std::string patches_dir();  // folder holding the *.bps patches
bool is_installed();  // installed with the patches shipped next to this program
bool needs_update();  // installed, but with an older release's patches

// Installs from `src` (the game folder, or a folder containing it). Returns "" on success, else a message for the
// player. Saves from an earlier install are kept.
std::string install(const std::string& src);

// For tests: apply one BPS patch. Returns false (and leaves `out` empty) if the source or patch doesn't match.
bool bps_apply(const std::string& src, const std::string& patch, std::string& out);

}  // namespace installer
