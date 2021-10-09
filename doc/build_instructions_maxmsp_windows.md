# Building the Max Externals on Windows

### Dependencies

The actual [libmapper][libmapper] repository is required to build the externals.

[libmapper]: https://github.com/libmapper/libmapper

libmapper depends on version 0.30 of liblo or later.
Please clone the [LibLo repository][liblo] and consult its documentation to build for Windows.

[liblo]: https://github.com/radarsat1/liblo

Cmake is also required to generate visual studio solutions, and can be installed [here][cmake]. Add it to the environment path when prompted for terminal access later on.

[cmake]: https://cmake.org/download/

Zlib is required as well, which you can pick up [from nuget][zlib].

[zlib]: https://www.nuget.org/packages/zlib-msvc14-x64/

Finally, you'll need Visual Studio 2017 or 2019, which you can grab [here][visual_studio]. Be sure to install the C++ developer tools when installing if you don't already have them.

[visual_studio]: https://visualstudio.microsoft.com/vs/

### Configuring

Once libmapper and liblo are downloaded and built, open a terminal in its root folder.

Create a build directory and cd into it

    mkdir ./build
    cd build

Modify the CMakeLists.txt file in the root folder, replacing the paths near the top with your local paths.

Run the following to generate a solution, replacing your version's details:

    cmake -G "Visual Studio 16 2019" ..

### Building

By now, you should have a Visual Studio solution in the ./build directory. Open the .sln and build the external project(s) of your choosing.
