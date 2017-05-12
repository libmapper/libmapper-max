echo cleaning up dest folder...
rm -r ./mapper-osx-max-pd

echo building mapper.mxo...
cd mapper/
xcodebuild build

echo building mapper.pd_darwin...
make clean
make

echo building map.device.mxo...
cd ../mapdevice/
xcodebuild build

echo building map.in.mxo...
cd ../mapin/
xcodebuild build

echo building map.out.mxo...
cd ../mapout/
xcodebuild build
cd ..

echo moving/copying files...

mkdir ./mapper-osx-max-pd
cd mapper-osx-max-pd
mkdir old_bindings
mkdir new_bindings
cd ..
cp AUTHORS ./mapper-osx-max-pd/
cp COPYING ./mapper-osx-max-pd/
cp README ./mapper-osx-max-pd/
cp -r oscmulticast ./mapper-osx-max-pd/

mv ./mapper/build/maxmsp/mapper.mxo ./mapper-osx-max-pd/old_bindings/
cp ./mapper/tester.json ./mapper-osx-max-pd/old_bindings/
cp ./help/mapper.maxhelp ./mapper-osx-max-pd/old_bindings/
cp ./mapper/mapper.pd_darwin ./mapper-osx-max-pd/old_bindings/
cp ./help/mapper.help.pd ./mapper-osx-max-pd/old_bindings/
cp ./oscmulticast/oscmulticast.mxo ./mapper-osx-max-pd/old_bindings/

mv ./mapdevice/build/maxmsp/map.device.mxo ./mapper-osx-max-pd/new_bindings/
cp ./help/map.device.maxhelp ./mapper-osx-max-pd/new_bindings/

mv ./mapin/build/maxmsp/map.in.mxo ./mapper-osx-max-pd/new_bindings/
cp ./help/map.in.maxhelp ./mapper-osx-max-pd/new_bindings/

mv ./mapout/build/maxmsp/map.out.mxo ./mapper-osx-max-pd/new_bindings/
cp ./help/map.out.maxhelp ./mapper-osx-max-pd/new_bindings/

cp ./help/logolink.maxpat ./mapper-osx-max-pd/old_bindings/
cp ./help/logolink.maxpat ./mapper-osx-max-pd/new_bindings/

echo copying dylibs...

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/old_bindings/mapper.mxo/Contents/MacOS/mapper -d ./mapper-osx-max-pd/old_bindings/mapper.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/new_bindings/map.device.mxo/Contents/MacOS/map.device -d ./mapper-osx-max-pd/new_bindings/map.device.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/new_bindings/map.in.mxo/Contents/MacOS/map.in -d ./mapper-osx-max-pd/new_bindings/map.in.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/new_bindings/map.out.mxo/Contents/MacOS/map.out -d ./mapper-osx-max-pd/new_bindings/map.out.mxo/Contents/libs/

echo Done.
