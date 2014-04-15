libmapper bindings for MaxMSP and Pure Data
===========================================

MAXMSP and PureData objects instantiating libmapper devices. Help patches for MaxMSP and
PureData are included for documentation.

The `mapper` object can be used with both MaxMSP and Pure Data - it encapsulates the
functionality of a libmapper device and works as a central hub in your patch by sending
and receiving labeled messages from the libmapper network.

For MaxMSP there are now another set of bindings which attempt to provide a more max-like
interface to the libmapper ecosystem. The `map.device` object creates a libmapper device
as before, but it communicates with an arbitrary number of `map.in` and `map.out` objects
in your patch (and subpatchers) which can be used essentially as networked replacements
for the internal `inlet` and `outlet` objects. Please load the help patches for more
documentation and examples of use.

Hopefully in the near future the new bindings will also be adapted for Pure Data - for
now Pd users are stuck with the (fully-functional) `mapper` object.

This software is licensed under the GNU Lesser Public General License version 2.1 or
later; see the attached file COPYING for details, which should be included in this
download.

To build, you will first need to install [libmapper][1] and [liblo][2] from their
respective repositories. An installation of liblo <= v0.26 is not sufficient, as
libmapper depends on new functionality in the library.

Joseph Malloch 2011
[Input Devices and Music Interaction Laboratory][3], McGill University.

[1]: http://github.com/libmapper/libmapper
[2]: http://github.com/radarsat1/liblo
[3]: http://idmil.org/software/libmapper