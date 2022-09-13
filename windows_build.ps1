# Create build directory
$scriptDir = Get-Location
if (!(Test-Path "$($scriptDir)/build/")) {
  mkdir build
}
if (!(Test-Path "$($scriptDir)/dist/")) {
  mkdir dist
}

# Download and install libmapper
if (!(Test-Path "$($scriptDir)/build/libmapper/")) {
  cd "$($scriptDir)/build"
  Invoke-WebRequest https://github.com/libmapper/libmapper/archive/refs/tags/2.3.zip -OutFile libmapper.zip
  Expand-Archive libmapper.zip libmapper
  rm libmapper.zip
  cd libmapper/libmapper-2.3
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

# Build the externals
cd "$($scriptDir)/build"
cmake ..
cmake --build . --target all_build

# Copy the externals to ./dist
cd $scriptDir
cp ./build/Debug/* ./dist

# Copy the help files over
if (!(Test-Path "$($scriptDir)/dist/help/")) {
  cd "$($scriptDir)/dist/"
  mkdir help
}
cd $scriptDir
cp ./mapper/mapper.maxhelp ./dist/help/
cp ./mpr_device/mpr.device.maxhelp ./dist/help/
cp ./mpr_in/mpr.in.maxhelp ./dist/help/
cp ./mpr_out/mpr.out.maxhelp ./dist/help/

Write-Host "Done: /dist/ contains the built externals"