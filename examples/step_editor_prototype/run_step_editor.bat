@echo off
rem ---------------------------------------------------------------
rem  WN3D STEP Conditioning Editor (prototype) launcher
rem  Usage:   run_step_editor.bat [path\to\file.step]
rem  You can also drag-and-drop a .step file onto this .bat.
rem  First run creates the Python environment with uv (one-time).
rem ---------------------------------------------------------------
setlocal
set "HERE=%~dp0"
set "PYW=%HERE%.venv\Scripts\pythonw.exe"

if not exist "%PYW%" (
    where uv >nul 2>nul
    if errorlevel 1 (
        echo ERROR: no .venv yet and uv is not on PATH.
        echo Install uv from https://docs.astral.sh/uv/ and run this again.
        pause
        exit /b 1
    )
    echo First run: creating the environment with uv ...
    uv sync --project "%HERE%."
    if errorlevel 1 (
        echo ERROR: uv sync failed.
        pause
        exit /b 1
    )
)

rem Launch detached with pythonw (no console window stays open).
start "" "%PYW%" "%HERE%step_editor.py" %*
endlocal
