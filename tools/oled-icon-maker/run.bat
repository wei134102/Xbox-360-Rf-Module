@echo off
cd /d "%~dp0"

REM 用当前 python 的 base_prefix 设置 Tcl/Tk（覆盖 CSR BlueSuite 等错误环境变量）
for /f "delims=" %%I in ('python -c "import sys; print(sys.base_prefix)" 2^>nul') do set "PYROOT=%%I"
if defined PYROOT (
  if exist "%PYROOT%\tcl\tcl8.6\init.tcl" (
    set "TCL_LIBRARY=%PYROOT%\tcl\tcl8.6"
    set "TK_LIBRARY=%PYROOT%\tcl\tk8.6"
  )
)

python -c "import PIL" 2>nul || pip install -r requirements.txt
python "%~dp0oled_icon_maker.py"
if errorlevel 1 pause
