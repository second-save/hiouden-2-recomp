#include "installer.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace installer {
namespace {

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool read_file(const fs::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

bool write_file(const fs::path& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(f);
}

uint32_t crc32(const std::string& s, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) {
        c ^= static_cast<uint8_t>(s[i]);
        for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}

uint32_t le32(const std::string& s, size_t at) {
    return static_cast<uint8_t>(s[at]) | static_cast<uint8_t>(s[at + 1]) << 8 |
           static_cast<uint8_t>(s[at + 2]) << 16 | static_cast<uint32_t>(static_cast<uint8_t>(s[at + 3])) << 24;
}

// name (lowercase) -> path, for the files directly inside `dir`
std::map<std::string, fs::path> listing(const fs::path& dir) {
    std::map<std::string, fs::path> m;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec))
        if (e.is_regular_file(ec)) m[lower(e.path().filename().string())] = e.path();
    return m;
}

bool looks_like_game(const fs::path& dir) {
    const auto m = listing(dir);
    return m.count("hio.exe") && m.count("hiosys.exe") && m.count("a.cha");
}

// the game folder itself, or one directly inside the folder the player picked
fs::path find_root(const fs::path& src) {
    if (looks_like_game(src)) return src;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(src, ec))
        if (e.is_directory(ec) && looks_like_game(e.path())) return e.path();
    return {};
}

bool is_save(const std::string& lname) {
    return (lname.size() > 4 && lname.compare(lname.size() - 4, 4, ".hio") == 0) || lname == "data.atr";
}

}  // namespace

std::string pref_dir() {
    if (const char* home = SDL_getenv("HIOUDEN_HOME")) {  // tests: never touch the player's real folder
        fs::create_directories(home);
        std::string h = home;
        return h.back() == '/' ? h : h + "/";
    }
    char* p = SDL_GetPrefPath("Second Save", "Hiouden II");
    std::string s = p ? p : "./";
    SDL_free(p);
    return s;
}

std::string game_dir() { return pref_dir() + "game/"; }

std::string patches_dir() {
    const char* base = SDL_GetBasePath();  // macOS app: Contents/Resources/; elsewhere the program's folder
    const std::string b = base ? base : "./";
    for (const std::string& cand : {b + "patches/", b + "../patches/", std::string("dist/patches/")})
        if (fs::is_directory(cand)) return cand;
    return b + "patches/";
}

bool is_installed() { return fs::exists(game_dir() + "install.ok") && looks_like_game(game_dir()); }

bool bps_apply(const std::string& src, const std::string& p, std::string& out) {
    out.clear();
    if (p.size() < 16 || p.compare(0, 4, "BPS1") != 0) return false;
    const size_t end = p.size() - 12;
    if (crc32(p, p.size() - 4) != le32(p, p.size() - 4)) return false;
    if (crc32(src, src.size()) != le32(p, p.size() - 12)) return false;  // not the original file
    size_t i = 4;
    auto num = [&]() -> uint64_t {
        uint64_t v = 0, shift = 1;
        while (i < p.size()) {
            const uint8_t x = static_cast<uint8_t>(p[i++]);
            v += (x & 0x7F) * shift;
            if (x & 0x80) break;
            shift <<= 7;
            v += shift;
        }
        return v;
    };
    const uint64_t slen = num(), tlen = num(), mlen = num();
    if (slen != src.size()) return false;
    i += mlen;
    std::string t;
    t.reserve(tlen);
    int64_t srel = 0, trel = 0;
    while (i < end) {
        const uint64_t d = num();
        const uint64_t cmd = d & 3, n = (d >> 2) + 1;
        if (cmd == 0) {
            if (t.size() + n > src.size()) return false;
            t.append(src, t.size(), n);
        } else if (cmd == 1) {
            if (i + n > end) return false;
            t.append(p, i, n);
            i += n;
        } else {
            const uint64_t o = num();
            const int64_t off = (o & 1) ? -static_cast<int64_t>(o >> 1) : static_cast<int64_t>(o >> 1);
            if (cmd == 2) {
                srel += off;
                if (srel < 0 || static_cast<uint64_t>(srel) + n > src.size()) return false;
                t.append(src, static_cast<size_t>(srel), n);
                srel += static_cast<int64_t>(n);
            } else {
                trel += off;
                if (trel < 0) return false;
                for (uint64_t k = 0; k < n; ++k) {  // byte by byte: the copy may overlap what it writes
                    if (static_cast<uint64_t>(trel) >= t.size()) return false;
                    t.push_back(t[static_cast<size_t>(trel++)]);
                }
            }
        }
    }
    if (t.size() != tlen || crc32(t, t.size()) != le32(p, p.size() - 8)) return false;
    out.swap(t);
    return true;
}

std::string install(const std::string& src_in) {
    const fs::path root = find_root(fs::path(src_in));
    if (root.empty())
        return "That folder doesn't hold Hiouden II (HIO.EXE, HIOSYS.EXE, A.CHA). Pick the folder the game is in.";
    const auto files = listing(root);

    // patch everything in memory first: a wrong or already patched file stops the install before any write
    std::vector<std::pair<std::string, std::string>> patched;  // file name as found, new bytes
    std::error_code ec;
    int n = 0;
    for (const auto& e : fs::directory_iterator(patches_dir(), ec)) {
        const std::string pname = e.path().filename().string();
        if (pname.size() < 5 || lower(pname).compare(pname.size() - 4, 4, ".bps") != 0) continue;
        const std::string target = lower(pname.substr(0, pname.size() - 4));
        const auto it = files.find(target);
        if (it == files.end()) return "Missing game file " + pname.substr(0, pname.size() - 4) + ".";
        std::string s, p, out;
        if (!read_file(it->second, s) || !read_file(e.path(), p)) return "Couldn't read " + it->first + ".";
        if (!bps_apply(s, p, out))
            return it->second.filename().string() +
                   " isn't the original Taiwanese DOS release (a different version, or already patched).";
        patched.emplace_back(it->second.filename().string(), std::move(out));
        ++n;
    }
    if (n == 0) return "The English patch files are missing (patches/ next to the program).";

    const fs::path dst = game_dir(), tmp = pref_dir() + "game.new";
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp, ec);
    for (const auto& [lname, path] : files) {  // the game folder is flat
        fs::copy_file(path, tmp / path.filename(), fs::copy_options::overwrite_existing, ec);
        if (ec) return "Couldn't copy " + path.filename().string() + ": " + ec.message();
    }
    for (const auto& [name, data] : patched)
        if (!write_file(tmp / name, data)) return "Couldn't write " + name + ".";
    if (fs::exists(dst)) {  // keep the save slots of an earlier install
        for (const auto& [lname, path] : listing(dst))
            if (is_save(lname)) fs::copy_file(path, tmp / path.filename(), fs::copy_options::overwrite_existing, ec);
        fs::remove_all(dst, ec);
    }
    fs::rename(tmp, dst, ec);
    if (ec) return "Couldn't finish the install: " + ec.message();
    write_file(dst / "install.ok", "Hiouden II English patch installed\n");
    return "";
}

}  // namespace installer
