# Create build directory
$scriptDir = Get-Location
if (!(Test-Path "$($scriptDir)/build/")) {
  mkdir build
}

# Download and install libmapper
if (!(Test-Path "$($scriptDir)/build/libmapper/")) {
  cd "$($scriptDir)/build"
  Invoke-WebRequest https://github.com/libmapper/libmapper/archive/refs/heads/main.zip -OutFile libmapper.zip
  Expand-Archive libmapper.zip libmapper
  rm libmapper.zip
  cd libmapper/libmapper-main
  ./windows_build.ps1
}

# Download the max sdk
if (!(Test-Path "$($scriptDir)/build/max-sdk/")) {
  cd "$($scriptDir)/build"
  Invoke-WebRequest https://github.com/Cycling74/max-sdk/releases/download/v8.2.0/max-sdk-8.2-release.zip -OutFile max-sdk.zip
  Expand-Archive max-sdk.zip max-sdk
  rm max-sdk.zip
}

# Build the externals
cd "$($scriptDir)/build"
cmake ..
cmake --build . --target all_build

Write-Host "Done: /build/Debug/ contains the built externals"
Pause