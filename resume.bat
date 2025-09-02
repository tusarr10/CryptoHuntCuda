@echo off
for /f "tokens=2 delims==" %%i in ('findstr START_KEY resume.txt') do set START_KEY=%%i
echo Resuming from %START_KEY%
CryptoHuntCuda --range %START_KEY%:ffffffffff  -m addresses --coin BTC -o found.txt -i hash160.bin -t 0 -g --gpui 0 --gpux 256,256
pause
