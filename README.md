# cwASIO

The cwASIO SDK ("**c**ompatible **w**ith **ASIO**", pronounced kwayz-eye-oh) is
an alternative implementation of an SDK for the ASIO driver API originally
defined by Steinberg[^1]. The cwASIO SDK is an independent implementation that
is compatible with the driver API of ASIO, while supporting it on Linux, in
addition to the native Windows implementation of ASIO. This allows building
portable audio applications that work on both operating systems.

[^1]: ASIO is a trademark of Steinberg Media Technologies GmbH.

By using cwASIO, audio applications can be built that don't depend on the ASIO
SDK from Steinberg, and therefore aren't bound to its license. Those
applications can be made portable between Linux and Windows.

cwASIO works with existing ASIO device drivers on Windows. It supports
enumeration of ASIO devices registered in the Windows registry. On Linux, the
driver interface consists of a set of functions exported by a dynamically loaded
shared object file, with a functionality that matches that on Windows.

The cwASIO API is a C interface, but a C++ wrapper is included for C++ projects.
`C++23` or newer is required for the wrapper, but for older C++ versions it is
fairly easy to use the C interface.

## Structure of cwASIO

The sources consist of a C header with type definitions, a header with the
native cwASIO C interface, a header with the C++ wrapper, and an ASIO
compatibility header to support easy porting of applications based on the
Steinberg ASIO SDK to cwASIO. Some C source files contain the library code.

Documentation is provided within the source files in doxygen format.

## Using cwASIO

For building a host application, cwASIO can be used as a library to link to, or
as a set of source files to integrate into the host application build process.
Using CMake makes this easy, as cwASIO contains the necessary CMakeLists.txt
files.

### Linking to the cwASIO library

TBC

### Integrating the sources into the host build

This uses the `FetchContent` module of CMake to fetch the cwASIO sources, and
make them a part of the host application build.

## APIs
### Compatible API

The compatible API attempts to mimick the original ASIO C API closely, so that
applications that have been built with the original ASIO SDK can change to
cwASIO with minimal effort. This applies to the core API, not the OS-specific
driver enumeration interface. This same API is also available on Linux.

The compatible API is declared in `asio.h`. It holds hidden global state so
the application can only have one driver loaded at any time, a restriction that
it shares with the original ASIO SDK.

This API is a thin C wrapper around the native API on each platform.

### Native API

You don't need to use the compatible API, there can be good reasons to use the
native API. The most immediate benefit is the possibility of loading and using
several drivers simultaneously. The native API is specific to cwASIO and is not
meant to be a drop-in replacement for the original ASIO API.

The native API is close to identical on Linux and Windows, because on Linux the
COM interface used on Windows is mimicked. The functionality is the same, so it
should not be difficult to accommodate either in a portable application.

The native API is declared in `cwASIO.h`. It is a C interface that can be
used in C++ when included inside an `extern "C" { ... }` bracket.

### C++ API

The C++ API is a thin wrapper around the native cwASIO C API, which converts it
to ordinary C++ objects and uses data types from the C++ standard library. This
includes error handling compatible with `system_error`, more automatic resource
management, and more.

This API is declared in `cwASIO.hpp`. Of course, you would omit the
`extern "C" { ... }` brackets in this case.

## Enumerating devices

Enumerating must be done with the native API, and is not compatible with the
ASIO SDK. The native API has a portable enumerating function, which uses an
enumeration method appropriate for the platform. It consists of a single function
`cwASIOenumerate()`, which accepts a callback that is called for each driver that
was found installed in the system.

Enumerating works by scanning through the registration database (The registry on
Windows, or the `/etc/cwASIO` directory on Linux) and calling the callback once
for each entry found. Three strings are being passed to the callback per entry:
The driver name, the driver loading id, and the driver description. On Windows,
the loading id is a CLSID, which is a GUID that can be resolved to the driver
DLL to load. On Linux, the loading id is the path to the driver's shared object
file.

Note that you enumerate the installed drivers, not the audio devices that are
actually connected and ready to be used! Whether an audio device is present and
operable can only be determined once its driver is loaded.

## ASIO compatibility details

cwASIO is a reimplemetation of the Steinberg ASIO API that does not rely on
Steinberg's ASIO SDK, thus avoiding infringing their copyright. Reimplementing
an API is commonly regarded as fair use; a high-profile court case concerning
this topic was decided in 2021 by the US Supreme Court, when Oracle sued Google
over the use of declarations for the Java API, and lost. On a much smaller
scale, cwASIO does this by providing a reimplementation of the ASIO driver API,
without using any of the files provided with the Steinberg ASIO SDK.

The reimplementation uses different type and function names from those in the
ASIO SDK, hence both can be used simultaneously by the same host application
without build errors, as long as the native cwASIO API is used. When using the
ASIO compatibility API, there will be conflicts, hence only one can be used at
the same time.

The Steinberg ASIO SDK supports only Windows. Support for the Mac has been
dropped with version 2.3, and other platforms weren't supported for a long time.
On Windows, ASIO depends on Microsoft COM, but there are deviations from the
COM rules that make it noncompliant. Consequently, an ASIO driver can be found
and enumerated on the system using COM functionality, but it doesn't present a
COM compliant API, due to issues with calling conventions. This is particularly
prominent on 32-bit Windows, where compatibility with C is severely impaired.

cwASIO regards 32-bit Windows as obsolete and thus takes advantage of the fact
that the most serious problems don't exist on 64-bit Windows, because of its
unified calling conventions. If you still must support 32-bit Windows, cwASIO
may not be suitable for you. Support for 32-bit Linux should be fine, however.

On Linux, the cwASIO API mimicks a COM interface to some extent, but since there
is no system support for finding COM components, cwASIO used a different
discovery mechanism based on entries in the `/etc/cwASIO` folder.

Owing to its pedigree, ASIO relies on a struct with two 32-bit numbers for
representing a 64-bit integer on the Windows platform. Even though 64-bit
integers have been available natively for a long time, backwards compatibility
has prevented ASIO from taking advantage of them on Windows. On other platforms,
no such compatibility problem exists, hence we take the opportunity to use the
native 64-bit integers. Writers of portable applications will have to take this
into account when using cwASIO.

The native cwASIO API is a C API that doesn't need C++ even on Windows. This
makes it more widely applicable, since host applications can be written in
languages with a C binding, which is much more common than a C++ binding. It
also makes writing portable applications easier, since the API is the same
everywhere. Furthermore, the native API allows a host application to have
more than one driver loaded and used at the same time, if so desired.
