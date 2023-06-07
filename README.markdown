# libmapper bindings for Max and Pure Data

Max and Pure Data (Pd) objects instantiating [libmapper][1] devices and signals. Help patches are included for documentation.

The `mapper` object can be used with both Max and Pd - it encapsulates the functionality of a libmapper device and works as a central hub in your patch by sending and receiving labeled messages from the libmapper network.

For Max there is also another set of bindings which attempt to provide a more Max-like interface to the libmapper ecosystem. The `mpr.device` object creates a libmapper device as before, but it communicates with an arbitrary number of `mpr.in` and `mpr.out` objects in your patch (and subpatchers) which can be used essentially as networked replacements
for the internal `inlet` and `outlet` objects. Please load the help patches for more documentation and examples of use.

Hopefully in the near future the new bindings will also be adapted for Pure Data - for now Pd users are stuck with the (fully-functional) `mapper` object.

This software is licensed under the GNU Lesser Public General License version 2.1 or later; see the attached file COPYING for details, which should be included in this download.

## Building

### MacOS

On MacOS you can then all the externals by running the script `package-osx.sh` from the command-line:

```
./macos-build.sh

```

This script will first download the Max SDK, and download and build the library dependencies (liblo, libmapper) as "universal binaries" (x86_64_apple_darwin and aarch64-apple-darwin).

Once the script has run the externals will be found in /dist/Max and dist/pd.

### Windows

For Windows machines, run the `./windows_build.ps1` script in this repository's root from powershell. The script will download and install the libmapper and liblo dependencies and build the externals. To use the Max externals on Windows, you'll need to copy the libmapper, liblo and zlib dll's into `C:\Program Files\Cycling '74\Max 8` or equivalent on your machine.

### Linux

Before building you will first need to install [libmapper][1] and [liblo][2] from their respective repositories. An installation of liblo < v0.30 is not sufficient, as libmapper depends on new functionality in the library.

Since Max is not available for Linux only the Pd bindings can be built. A makefile is included in the `mapper` directory:

```
cd mapper
make

```

## Acknowledgements

Development of this software was supported by the [Input Devices and Music Interaction Laboratory][3] at McGill University and the [Graphics and Experiential Media (GEM) Lab][4] at Dalhousie University.

[1]: http://github.com/libmapper/libmapper
[2]: http://github.com/radarsat1/liblo
[3]: http://idmil.org/software/libmapper
[4]: https://gem.cs.dal.ca/
