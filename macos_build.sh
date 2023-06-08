echo cleaning up dist folder...
rm -r ./dist

mkdir ./dist
mkdir ./dist/Max
mkdir ./dist/Max/Mapper
mkdir ./dist/Max/Mapper/externals
mkdir ./dist/Max/Mapper/extras
mkdir ./dist/Max/Mapper/help
mkdir ./dist/Max/Mapper/init
mkdir ./dist/Max/Mapper/media
mkdir ./dist/Max/Mapper/support
mkdir ./dist/pd
mkdir ./dist/pd/mapper

cp AUTHORS ./dist/
cp COPYING ./dist/
cp README ./dist/
cp README.markdown ./dist/Max/Mapper/readme.md
cp COPYING ./dist/Max/Mapper/license.txt
cp ./maxdist/mpr-objectmappings.txt ./dist/Max/Mapper/init/
cp ./maxdist/mpr.overview.maxpat ./dist/Max/Mapper/extras/
cp ./maxdist/package-info.json ./dist/Max/Mapper/
cp -r ./maxdist/docs ./dist/Max/Mapper/
cp -r ./maxdist/examples ./dist/Max/Mapper/
cp ./icons/icon.png ./dist/Max/Mapper/

if [ ! -d "./build" ]
then
    mkdir build
fi

cd ./build
mkdir inst

if [ -f "./inst/lib/liblo.7.dylib" ]
then
    echo liblo already installed
else
    echo installing liblo
    if [ -d "./liblo" ]
    then
        echo liblo already downloaded
    else
        echo downloading liblo...
        curl -L -O https://downloads.sourceforge.net/project/liblo/liblo/0.31/liblo-0.31.tar.gz
        tar -xzf liblo-0.31.tar.gz
        rm liblo-0.31.tar.gz
        mv liblo-0.31 liblo
    fi
    cd ./liblo
    echo build liblo: arm
    (./configure CFLAGS="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk --target=arm64-apple-darwin -fPIC" CXXFLAGS="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk --target=arm64-apple-darwin" --build=x86_64-apple-darwin19.6.0 --host=aarch64-apple-darwin --prefix=$PWD/../inst --disable-tests --disable-tools --disable-examples || (cat config.log; false))
    make install
    cd src
    gcc -dynamiclib -Wl,-undefined -Wl,dynamic_lookup -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk --target=arm64-apple-darwin -o .libs/liblo.7.dylib  .libs/liblo_la-address.o .libs/liblo_la-send.o .libs/liblo_la-message.o .libs/liblo_la-server.o .libs/liblo_la-method.o .libs/liblo_la-blob.o .libs/liblo_la-bundle.o .libs/liblo_la-timetag.o .libs/liblo_la-pattern_match.o .libs/liblo_la-version.o .libs/liblo_la-server_thread.o -O0 -g -install_name  ../inst/lib/liblo.7.dylib -compatibility_version 12 -current_version 12.1 -Wl,-single_module
    cd ..
    mv ./src/.libs/liblo.7.dylib ./../inst/lib/liblo.7.dylib.arm64
    echo build liblo: x86_64
    make clean
    (./configure --host=$HOST --prefix=$PWD/../inst --disable-tests --disable-tools --disable-examples CFLAGS=-fPIC || (cat config.log; false))
    make install
    mv ./src/.libs/liblo.7.dylib ./../inst/lib/liblo.7.dylib.x86_64
    echo build liblo universal binary
    cd ./../inst/lib
    lipo -create -output ./liblo.7.dylib ./liblo.7.dylib.arm64 ./liblo.7.dylib.x86_64
    lipo -info ./liblo.7.dylib
    rm ./liblo.7.dylib.*
    cd ../..
fi

echo PWD: $PWD

if [ -f "./inst/lib/libmapper.11.dylib" ]
then
    echo libmapper already installed
else
    echo installing libmapper
    if [ -d "./libmapper" ]
    then
        echo libmapper already downloaded
    else
        echo downloading libmapper...
        curl -L -O https://github.com/libmapper/libmapper/releases/download/2.4.1/libmapper-2.4.tar.gz
        tar -xzf libmapper-2.4.tar.gz
        rm libmapper-2.4.tar.gz
        mv libmapper-2.4 libmapper
    fi
    cd libmapper
    echo building libmapper: arm
    ./autogen.sh CFLAGS="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk --target=arm64-apple-darwin" CXXFLAGS="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk --target=arm64-apple-darwin" --build=x86_64-apple-darwin19.6.0 --host=aarch64-apple-darwin --prefix=$PWD/../inst PKG_CONFIG_PATH=$PWD/../inst/lib/pkgconfig
    make install
    echo trying to fix error
    cd ./src/.libs
    clang -dynamiclib -Wl,-undefined -Wl,dynamic_lookup -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk --target=arm64-apple-darwin -o libmapper.11.dylib libmapper_la-device.o libmapper_la-expression.o libmapper_la-graph.o libmapper_la-link.o libmapper_la-list.o libmapper_la-map.o libmapper_la-message.o libmapper_la-network.o libmapper_la-object.o libmapper_la-path.o libmapper_la-property.o libmapper_la-signal.o libmapper_la-slot.o libmapper_la-table.o libmapper_la-time.o libmapper_la-mpr_set_coerced.o libmapper_la-value.o -L$PWD/../../../inst/lib $PWD/../../../inst/lib/liblo.dylib -lz -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk -install_name $PWD/../../../inst/lib/libmapper.11.dylib -compatibility_version 10 -current_version 10.0 -Wl,-single_module
    cd ../..
    mv ./src/.libs/libmapper.11.dylib ./../inst/lib/libmapper.11.dylib.arm64
    file ./../inst/lib/libmapper.11.dylib.arm64

    echo building libmapper: x86_64
    make clean
    ./autogen.sh --prefix=$PWD/../inst PKG_CONFIG_PATH=$PWD/../inst/lib/pkgconfig --disable-python --disable-java --disable-csharp --disable-examples --disable-docs --disable-tests
    make install
    mv ./../inst/lib/libmapper.11.dylib ./../inst/lib/libmapper.11.dylib.x86_64
    echo libmapper: building universal binary
    cd ./../inst/lib
    file ./libmapper.11.dylib.arm64
    file ./libmapper.11.dylib.x86_64
    lipo -create -output ./libmapper.11.dylib ./libmapper.11.dylib.arm64 ./libmapper.11.dylib.x86_64
    file ./libmapper.11.dylib
    rm ./libmapper.11.dylib.*
    cd ../..
fi

if [ -d "./max-sdk" ]
then
    echo Max SDK already downloaded
else
    echo downloading Max SDK
    curl -L -O https://github.com/Cycling74/max-sdk/releases/download/v8.2.0/max-sdk-8.2-release.zip
    unzip max-sdk-8.2-release.zip
    rm max-sdk-8.2-release.zip
fi

if [ -d "./pure-data" ]
then
    echo Pure Data already downloaded
else
    echo downloading Pure Data
    curl -L -O https://github.com/pure-data/pure-data/archive/refs/tags/0.52-2.zip
    unzip 0.52-2.zip
    rm 0.52-2.zip
    mv pure-data-0.52-2 pure-data
fi

if [ -f "./../dylibbundler/dylibbundler" ]
then
    echo dylibbundler already built
else
    echo building dylibbundler
    cd ./../dylibbundler
    make
    cd ../build
fi

echo building externals
cmake ..
cmake --build .

echo bundling dylibs
./../dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mapper.mxo/Contents/MacOS/mapper -d ./mapper.mxo/Contents/libs/
./../dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mpr.device.mxo/Contents/MacOS/mpr.device -d ./mpr.device.mxo/Contents/libs/
./../dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mpr.in.mxo/Contents/MacOS/mpr.in -d ./mpr.in.mxo/Contents/libs/
./../dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./mpr.out.mxo/Contents/MacOS/mpr.out -d ./mpr.out.mxo/Contents/libs/
./../dylibbundler/dylibbundler -cd -b -p '@loader_path/../libs/' -x ./oscmulticast.mxo/Contents/MacOS/oscmulticast -d ./oscmulticast.mxo/Contents/libs/

echo copying files to dist
mv mapper/Debug/mapper.pd_darwin ../dist/pd/mapper/
cp ../mapper/mapper.help.pd ../dist/pd/mapper/

mv mapper.mxo ../dist/Max/Mapper/externals
cp ../mapper/mapper.maxhelp ../dist/Max/Mapper/help/
cp ../mapper/sample_device_definition.json ../dist/Max/Mapper/help/

mv mpr.device.mxo ../dist/Max/Mapper/externals
cp ../mpr.device/mpr.device.maxhelp ../dist/Max/Mapper/help/

mv mpr.in.mxo ../dist/Max/Mapper/externals
cp ../mpr.in/mpr.in.maxhelp ../dist/Max/Mapper/help/

mv mpr.out.mxo ../dist/Max/Mapper/externals
cp ../mpr.out/mpr.out.maxhelp ../dist/Max/Mapper/help/

mv oscmulticast.mxo ../dist/Max/Mapper/externals
cp ../oscmulticast/oscmulticast.maxhelp ../dist/Max/Mapper/help/

echo Done.
