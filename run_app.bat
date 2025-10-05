@echo off
echo Starting Streamlit Application on port 5001...
echo.
echo The browser will open automatically in 10 seconds...
echo.
echo Installing Python dependencies...
echo.

python -m pip install --user streamlit plotly scikit-learn pandas numpy joblib paho-mqtt pymodbustcp requests streamlit-webrtc opencv-python av

start "Streamlit App" python -m streamlit run ./HtierApp/app.py --server.port 5001

timeout /t 10 /nobreak

start http://localhost:5001