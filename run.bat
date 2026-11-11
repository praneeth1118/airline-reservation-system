@echo off
echo ========================================================
echo   Starting Airline Reservation System (C++ Backend)
echo ========================================================

if not exist airline_server.exe (
    echo airline_server.exe not found! Building first...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
)

echo Starting server on http://localhost:8080 ...
start http://localhost:8080
.\airline_server.exe

