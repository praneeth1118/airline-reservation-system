@echo off
echo ========================================================
echo   Compiling Airline Reservation System (C++ Backend)
echo ========================================================

g++ -std=c++11 -O2 ^
    src/SeatClass.cpp ^
    src/Passenger.cpp ^
    src/Ticket.cpp ^
    src/Flight.cpp ^
    src/AirlineManager.cpp ^
    src/ExcelExporter.cpp ^
    src/HttpServer.cpp ^
    src/main.cpp ^
    -o airline_server.exe ^
    -lws2_32

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] airline_server.exe built successfully!
    echo Run run.bat or .\airline_server.exe to launch the server.
) else (
    echo.
    echo [ERROR] Build failed. Please check compiler errors above.
)

