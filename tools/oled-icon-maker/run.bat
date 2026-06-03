@echo off
cd /d "%~dp0"

REM 修复部分 Windows 环境找不到 Tcl/Tk 的问题
for /f "delims=" %%P in ('where python 2^>nul') do set "PYEXE=%%P"
if not defined PYEXE (
  echo 未找到 python，请先安装 Python 3.8+
  pause
  exit /b 1
)
for %%D in ("%PYEXE%") do set "PYROOT=%%~dpD"
if exist "%PYROOT%tcl\tcl8.6" (
  set "TCL_LIBRARY=%PYROOT%tcl\tcl8.6"
  set "TK_LIBRARY=%PYROOT%tcl\tk8.6"
)

python -c "import PIL" 2>nul || pip install -r requirements.txt
python oled_icon_maker.py
pause
