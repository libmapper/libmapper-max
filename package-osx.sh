echo cleaning up dest folder...
rm -r ./mapper-osx-max-pd

echo building mapper.mxo...
cd mapper/
xcodebuild build

echo building mapper.pd_darwin...
make clean
make

echo building mpr.device.mxo...
cd ../mpr_device/
xcodebuild build

echo building mpr.in.mxo...
cd ../mpr_in/
xcodebuild build

echo building mpr.out.mxo...
cd ../mpr_out/
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

mv ./mpr_device/build/maxmsp/mpr.device.mxo ./mapper-osx-max-pd/new_bindings/
cp ./help/mpr.device.maxhelp ./mapper-osx-max-pd/new_bindings/

mv ./mpr_in/build/maxmsp/mpr.in.mxo ./mapper-osx-max-pd/new_bindings/
cp ./help/mpr.in.maxhelp ./mapper-osx-max-pd/new_bindings/

mv ./mpr_out/build/maxmsp/mpr.out.mxo ./mapper-osx-max-pd/new_bindings/
cp ./help/mpr.out.maxhelp ./mapper-osx-max-pd/new_bindings/

cp ./help/logolink.maxpat ./mapper-osx-max-pd/old_bindings/
cp ./help/logolink.maxpat ./mapper-osx-max-pd/new_bindings/

echo copying dylibs...

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/old_bindings/mapper.mxo/Contents/MacOS/mapper -d ./mapper-osx-max-pd/old_bindings/mapper.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/new_bindings/mpr.device.mxo/Contents/MacOS/mpr.device -d ./mapper-osx-max-pd/new_bindings/mpr.device.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/new_bindings/mpr.in.mxo/Contents/MacOS/mpr.in -d ./mapper-osx-max-pd/new_bindings/mpr.in.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/new_bindings/mpr.out.mxo/Contents/MacOS/mpr.out -d ./mapper-osx-max-pd/new_bindings/mpr.out.mxo/Contents/libs/

./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper-osx-max-pd/oscmulticast/oscmulticast.mxo/Contents/MacOS/oscmulticast -d ./mapper-osx-max-pd/oscmulticast/oscmulticast.mxo/Contents/libs/

cp -r ./mapper-osx-max-pd/oscmulticast/oscmulticast.mxo ./mapper-osx-max-pd/old_bindings/

echo Done.
