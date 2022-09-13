# libmapper bindings for MaxMSP and Pure Data

Max and Pure Data (Pd) objects instantiating [libmapper][1] devices. Help patches are included for documentation.

The `mapper` object can be used with both Max and Pd - it encapsulates the functionality of a libmapper device and works as a central hub in your patch by sending and receiving labeled messages from the libmapper network.

For MaxMSP there are now another set of bindings which attempt to provide a more Max-like interface to the libmapper ecosystem. The `mpr.device` object creates a libmapper device as before, but it communicates with an arbitrary number of `mpr.in` and `mpr.out` objects in your patch (and subpatchers) which can be used essentially as networked replacements
for the internal `inlet` and `outlet` objects. Please load the help patches for more documentation and examples of use.

Hopefully in the near future the new bindings will also be adapted for Pure Data - for now Pd users are stuck with the (fully-functional) `mapper` object.

This software is licensed under the GNU Lesser Public General License version 2.1 or later; see the attached file COPYING for details, which should be included in this download.

## Building

To build on OSX, you will first need to install [libmapper][1] and [liblo][2] from their respective repositories. An installation of liblo < v0.30 is not sufficient, as libmapper depends on new functionality in the library. You can then build all the binaries by running the script `package-osx.sh` from the command-line and the externals will be found in /dist/Max and dist/pd.

For Windows machines, run the `./windows_build.ps1` script in this repository's root from powershell. The script will download and install the libmapper and liblo dependencies and build the externals. To use the Max externals on Windows, you'll need to copy the libmapper, liblo and zlib dll's into `C:\Program Files\Cycling '74\Max 8` or equivalent on your machine.

## Acknowledgements

Development of this software was supported by the [Input Devices and Music Interaction Laboratory][3] at McGill University and the [Graphics and Experiential Media (GEM) Lab][4] at Dalhousie University.

[1]: http://github.com/libmapper/libmapper
[2]: http://github.com/radarsat1/liblo
[3]: http://idmil.org/software/libmapper
[4]: https://gem.cs.dal.ca/
