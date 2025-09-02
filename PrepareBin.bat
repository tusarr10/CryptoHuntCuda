@echo off
:: build_hash160.bat
:: Runs extract_hash160.py and BinSort to create hash160.bin for CryptoHunt-Cuda
:: Usage: Double-click or run in CMD

echo.
echo 🛠️  Building hash160.bin for multi-address search...
echo ====================================================

:: Check if Python is available
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Error: Python not found in PATH.
    echo    Install Python from https://www.python.org
    echo    Or ensure 'python' command works in CMD.
    pause
    exit /b 1
)

:: Check required files
if not exist "extract_hash160.py" (
    echo ❌ Missing: extract_hash160.py
    echo    Please place extract_hash160.py in this folder.
    pause
    exit /b 1
)

if not exist "addresses.txt" (
    echo ❌ Missing: addresses.txt
    echo    Create a file called 'addresses.txt' with one Bitcoin address per line.
    echo    Example:
    echo    1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
    echo    3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy
    echo    bc1qar0srrr7xfkvy5l643lydnw9re583dgvzevca5
    pause
    exit /b 1
)

if not exist "BinSort.exe" (
    echo ❌ Missing: BinSort.exe
    echo    Place BinSort.exe in this folder.
    pause
    exit /b 1
)

:: Step 1: Run extract_hash160.py
echo.
echo 📥 Step 1: Extracting hash160 from addresses...
python extract_hash160.py addresses.txt hash160_unsorted.bin
if %errorlevel% neq 0 (
    echo ❌ Python script failed.
    pause
    exit /b 1
)

:: Step 2: Run BinSort
echo.
echo 📊 Step 2: Sorting hash160_unsorted.bin → hash160.bin
BinSort.exe 20 hash160_unsorted.bin hash160.bin
if %errorlevel% neq 0 (
    echo ❌ BinSort failed. Check if BinSort.exe is valid.
    del hash160_unsorted.bin 2>nul
    pause
    exit /b 1
)

:: Cleanup
del hash160_unsorted.bin
if exist hash160_unsorted.bin (
    echo ⚠️  Warning: Could not delete temp file.
)

:: Success
echo.
echo ✅ Success! hash160.bin created and sorted.
echo    You can now use it with CryptoHunt-Cuda:
echo    CryptoHuntCuda -i hash160.bin -m addresses --coin BTC [other options]

echo.
pause