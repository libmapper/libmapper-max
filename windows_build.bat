@echo off
:: Create build directory
if not exist build\ mkdir build
cd build\
:: Download and build libmapper
if not exist libmapper\ (
  echo Downloading and extracting libmapper into build/ folder...
  powershell -Command "Invoke-WebRequest https://github.com/libmapper/libmapper/archive/refs/heads/main.zip -OutFile libmapper.zip"
  powershell -Command "Expand-Archive libmapper.zip libmapper"
  del libmapper.zip
  cd libmapper
  call windows_build.bat
  cd ..
)

:: Download the max-sdk
if not exist max-sdk\ (
  echo Downloading and extracting the max-sdk into build/ folder...
  powershell -Command "Invoke-WebRequest https://github.com/Cycling74/max-sdk/releases/download/v8.2.0/max-sdk-8.2-release.zip -OutFile max-sdk.zip"
  powershell -Command "Expand-Archive max-sdk.zip max-sdk"
  del max-sdk.zip
)

:: Build libmapper max externals
powershell -Command "cmake .."
powershell -Command "cmake --build . --target all_build"

echo Done! dll's for libmapper, liblo and zlib are located in the build/ folder
echo Find the build externals in build/Debug
pause