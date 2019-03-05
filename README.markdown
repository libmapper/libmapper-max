# libmapper bindings for MaxMSP and Pure Data

Max and Pure Data (Pd) objects instantiating [libmapper][1] devices. Help patches are included for documentation.

The `mapper` object can be used with both Max and Pd - it encapsulates the functionality of a libmapper device and works as a central hub in your patch by sending and receiving labeled messages from the libmapper network.

For MaxMSP there are now another set of bindings which attempt to provide a more Max-like interface to the libmapper ecosystem. The `map.device` object creates a libmapper device as before, but it communicates with an arbitrary number of `map.in` and `map.out` objects in your patch (and subpatchers) which can be used essentially as networked replacements
for the internal `inlet` and `outlet` objects. Please load the help patches for more documentation and examples of use.

Hopefully in the near future the new bindings will also be adapted for Pure Data - for now Pd users are stuck with the (fully-functional) `mapper` object.

This software is licensed under the GNU Lesser Public General License version 2.1 or later; see the attached file COPYING for details, which should be included in this download.

To build, you will first need to install [libmapper][1] and [liblo][2] from their respective repositories. An installation of liblo <= v0.26 is not sufficient, as libmapper depends on new functionality in the library.

On OSX you can build all the binaries by running the script `package-osx.sh` from the command-line.

## Acknowledgements

Development of this software was mainly supported by the [Input Devices and Music Interaction Laboratory][3] at McGill University.

[1]: http://github.com/libmapper/libmapper
[2]: http://github.com/radarsat1/liblo
[3]: http://idmil.org/software/libmapper
