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
  cd pure-data
  mkdir help
  cd ..
  cd max-8
  mkdir Mapper
  cd Mapper
  mkdir externals
  mkdir examples
  mkdir init
  mkdir media
  mkdir docs
  mkdir help
  mkdir support
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

# Start the dist process
cd $scriptDir
cp ./AUTHORS ./dist/
cp ./COPYING ./dist/
cp ./README ./dist/
cp ./maxdist/mpr-objectmappings.txt ./dist/max-8/Mapper/init/
cp ./maxdist/package-info.json ./dist/max-8/Mapper/
cp ./maxdist/examples/* ./dist/max-8/Mapper/examples/
cp ./icons/icon.png ./dist/max-8/Mapper/

# Copy the externals to dist
cd $scriptDir
cp ./build/Debug/mapper.mxe64 ./dist/max-8/Mapper/externals/mapper.mxe64
cp ./build/Debug/mpr.device.mxe64 ./dist/max-8/Mapper/externals/mpr.device.mxe64
cp ./build/Debug/mpr.in.mxe64 ./dist/max-8/Mapper/externals/mpr.in.mxe64
cp ./build/Debug/mpr.out.mxe64 ./dist/max-8/Mapper/externals/mpr.out.mxe64
cp ./build/mapper/Debug/mapper.dll ./dist/pure-data/mapper.dll

# Copy the help and docs
cp ./mapper/mapper.maxhelp ./dist/max-8/Mapper/help/
cp ./mapper/sample_device_definition.json ./dist/max-8/Mapper/media/
cp ./mpr.device/mpr.device.maxhelp ./dist/max-8/Mapper/help/
cp ./mpr.in/mpr.in.maxhelp ./dist/max-8/Mapper/help/
cp ./mpr.out/mpr.out.maxhelp ./dist/max-8/Mapper/help/
cp ./maxdist/docs/refpages/* ./dist/max-8/Mapper/docs/
cp ./mapper/mapper.help.pd ./dist/pure-data/help

# Copy the dlls to dist/max-8/support
cp ./build/libmapper/libmapper-main/dist/*.dll ./dist/max-8/Mapper/support/

Write-Host "Done: /dist/ contains the built externals"