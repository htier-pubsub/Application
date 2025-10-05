@echo off
echo Installing Python dependencies...
pip install requests

echo.
echo Starting Python HTTP Bridge...
echo Make sure your rust-app.exe is running first!
start ..\target\debug\saik-app
echo.

python stream_bridge_http.py

pause