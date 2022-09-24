# Create build directory
$scriptDir = Get-Location
if (!(Test-Path "$($scriptDir)/build/")) {
  mkdir build
}
if (!(Test-Path "$($scriptDir)/dist/")) {
  mkdir dist
  cd dist
  mkdir max-8
  mkdir pure-data
}

# Download and compile libmapper
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

# Build the externals
cd "$($scriptDir)/build"
cmake ..
cmake --build . --target all_build

# Copy the externals to dist
cd $scriptDir
cp ./build/Debug/* ./dist/max-8
cp ./build/mapper/Debug/mapper.dll ./dist/pure-data

# Copy the dlls to dist
cp ./build/libmapper/libmapper-main/dist/*.dll ./dist

# Copy the max help files to dist
if (!(Test-Path "$($scriptDir)/dist/max-8/help/")) {
  cd "$($scriptDir)/dist/max-8"
  mkdir help
  cd ..
  cd pure-data
  mkdir help
}
cd $scriptDir
cp ./mapper/mapper.maxhelp ./dist/max-8/help/
cp ./mpr_device/mpr.device.maxhelp ./dist/max-8/help/
cp ./mpr_in/mpr.in.maxhelp ./dist/max-8/help/
cp ./mpr_out/mpr.out.maxhelp ./dist/max-8/help/
cp ./mapper/mapper.help.pd ./dist/pure-data/help


Write-Host "Done: /dist/ contains the built externals"