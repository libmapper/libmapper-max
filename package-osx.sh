echo cleaning up dest folder...
rm -r ./dist

mkdir ./dist
mkdir ./dist/Max
mkdir ./dist/Max/Mapper
mkdir ./dist/Max/Mapper/externals
mkdir ./dist/Max/Mapper/extra
mkdir ./dist/Max/Mapper/help
mkdir ./dist/Max/Mapper/init
mkdir ./dist/Max/Mapper/media
mkdir ./dist/pd
mkdir ./dist/pd/mapper

cp AUTHORS ./dist/
cp COPYING ./dist/
cp README ./dist/
cp ./maxdist/mpr-objectmappings.txt ./dist/Max/Mapper/init/
cp ./maxdist/package-info.json ./dist/Max/Mapper/
cp ./maxdist/logolink.maxpat ./dist/Max/Mapper/help/
cp ./icons/icon.png ./dist/Max/Mapper/
cp ./test/instanced_sliders.maxpat ./dist/Max/Mapper/extra/
cp ./test/slider_instance.maxpat ./dist/Max/Mapper/extra/
cp ./test/instanced_pwm.maxpat ./dist/Max/Mapper/extra/
cp ./test/pwm_instance.maxpat ./dist/Max/Mapper/extra/

echo building mapper.mxo...
cd mapper/
xcodebuild build
cd ..
mv ./mapper/build/maxmsp/mapper.mxo ./dist/Max/Mapper/externals/
cp ./mapper/sample_device_definition.json ./dist/Max/Mapper/media/
cp ./mapper/mapper.maxhelp ./dist/Max/Mapper/help/
./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./dist/Max/Mapper/externals/mapper.mxo/Contents/MacOS/mapper -d ./dist/Max/Mapper/externals/mapper.mxo/Contents/libs/

echo building mapper.pd_darwin...
cd mapper/
make clean
make
cd ..
cp ./mapper/mapper.pd_darwin ./dist/pd/mapper/
cp ./mapper/mapper.help.pd ./dist/pd/mapper/

echo building mpr.device.mxo...
cd mpr_device/
xcodebuild build
cd ..
mv ./mpr_device/build/maxmsp/mpr.device.mxo ./dist/Max/Mapper/externals/
cp ./mpr_device/mpr.device.maxhelp ./dist/Max/Mapper/help/
./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./dist/Max/Mapper/externals/mpr.device.mxo/Contents/MacOS/mpr.device -d ./dist/Max/Mapper/externals/mpr.device.mxo/Contents/libs/

echo building mpr.in.mxo...
cd mpr_in/
xcodebuild build
cd ..
mv ./mpr_in/build/maxmsp/mpr.in.mxo ./dist/Max/Mapper/externals/
cp ./mpr_in/mpr.in.maxhelp ./dist/Max/Mapper/help/
./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./dist/Max/Mapper/externals/mpr.in.mxo/Contents/MacOS/mpr.in -d ./dist/Max/Mapper/externals/mpr.in.mxo/Contents/libs/

echo building mpr.out.mxo...
cd mpr_out/
xcodebuild build
cd ..
mv ./mpr_out/build/maxmsp/mpr.out.mxo ./dist/Max/Mapper/externals/
cp ./mpr_out/mpr.out.maxhelp ./dist/Max/Mapper/help/
./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./dist/Max/Mapper/externals/mpr.out.mxo/Contents/MacOS/mpr.out -d ./dist/Max/Mapper/externals/mpr.out.mxo/Contents/libs/

echo copying oscmulticast...
cp -r ./oscmulticast/oscmulticast.mxo ./dist/Max/Mapper/externals/
cp ./oscmulticast/oscmulticast.maxhelp ./dist/Max/Mapper/help/
./dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./dist/Max/Mapper/externals/oscmulticast.mxo/Contents/MacOS/oscmulticast -d ./dist/Max/Mapper/externals/oscmulticast.mxo/Contents/libs/

echo Done.
