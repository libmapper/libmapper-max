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

# Download the Pure Data and pd.build repos
if (!(Test-Path "$($scriptDir)/build/pure-data/")) {
  cd "$($scriptDir)/build"
  Invoke-WebRequest https://github.com/pure-data/pure-data/archive/refs/tags/0.52-2.zip -OutFile pure-data.zip
  Expand-Archive pure-data.zip pure-data
  rm pure-data.zip
}
if (!(Test-Path "$($scriptDir)/build/pd.build/")) {
  cd "$($scriptDir)/build"
  Invoke-WebRequest https://github.com/pierreguillot/pd.build/archive/refs/heads/master.zip -OutFile pd.build.zip
  Expand-Archive pd.build.zip pd.build
  rm pd.build.zip
}

# Build the externals
cd "$($scriptDir)/build"
cmake ..
cmake --build . --target all_build

# Copy the help files over
if (!(Test-Path "$($scriptDir)/build/Debug/help/")) {
  cd "$($scriptDir)/build/Debug/"
  mkdir help
}
cd $scriptDir
cp ./mapper/mapper.maxhelp ./build/Debug/help/
cp ./mpr_device/mpr.device.maxhelp ./build/Debug/help/
cp ./mpr_in/mpr.in.maxhelp ./build/Debug/help/
cp ./mpr_out/mpr.out.maxhelp ./build/Debug/help/

Write-Host "Done: /build/Debug/ contains the built externals"
Pause