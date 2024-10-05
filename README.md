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

### The enumeration and instantiation process on Windows

On Windows, enumerating ASIO drivers is done by scanning through the Windows
Registry. Each installed ASIO driver is represented by a key inside
`HKEY_LOCAL_MACHINE\SOFTWARE\ASIO`. The key name itself is arbitrary text, but
inside a set of values are required:

- A value named `CLSID` that contains a GUID in its textual representation,
  within curly braces. This is the class ID of the driver, which must be unique
  for each driver.
- A value named `Description` that contains a short text that can be presented
  to a user to identify the driver.

For each registered driver, those two values are presented to the caller of
`cwASIOenumerate()`, along with the key name. The class ID can be used to locate
the driver DLL in the file system, a process implemented by the `cwASIOload()`
function, which does the following:

- The GUID of the class ID is used to find the corresponding entry in the
  Windows Registry key `HKEY_CLASSES_ROOT\CLSID`.
- The entry found must contain the following subkeys: `InprocServer32`, `ProgID`
  and `Version`. All three contain default values, the first one additionally
  contains a value named `ThreadingModel`. All this is mandated by Microsoft
  COM.
- The full path to the driver DLL is contained in the default value of
  `InprocServer32`. It is used to load the DLL into memory and initialize it.
- The DLL is called to produce a class factory, which is an object used to
  create other objects.
- The class factory is used to create a driver instance, which is the object
  that implements the ASIO driver functionality.

Once the driver instance is created, it must be initialized with a call to its
`init()` method. At that point, the driver typically checks if the hardware is
present and functional, and if so initializes it. Thereafter, the driver should
be functional.

There is one more complication on Windows: Applications can be 32-bit or 64-bit,
and both may run on a 64-bit Windows host. Each of them needs its own driver,
i.e. you must provide the same driver in a 64-bit version and a 32-bit version.
Typically, you will compile the same driver source code twice, once in 32-bit
mode and once in 64-bit mode. Both have the same CLSID, but they need to be
registered separately.

Even though the two versions are registered in different locations in the
Windows Registry, Windows makes it appear to applications as if the path was the
same in both cases. A 32-bit application sees the 32-bit registration
information, and a 64-bit application sees the 64-bit registration information.
Therefore, enumeration is exactly the same for both kinds of application, and
they automatically see the appropriate information.

It is possible that an ASIO driver is only available in one of the two variants,
in which case only the matching applications can use it. If you only have a
32-bit driver, only 32-bit applications can use it, and likewise with 64-bit.

Note that when looking at the Registry with a 64-bit tool, the 32-bit entries
are visible under `HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\ASIO` and
`HKEY_CLASSES_ROOT\WOW6432Node\CLSID`.

### The enumeration and instantiation process on Linux

On Linux, cwASIO drivers are registered in `/etc/cwASIO`. Each driver is
described by a subdirectory therein. The name of the subdirectory is the
equivalent of the key name under `HKEY_LOCAL_MACHINE\SOFTWARE\ASIO` in the
Windows Registry. Each subdirectory contains at least the following two files:

- `driver` contains the full path to the driver shared object file as the ID
  string.
- `description` contains a brief descriptive text intended to be presented to a
  user for identifying the driver.

For each subdirectory, the contents of those two files are presented to the
caller of `cwASIOenumerate()`, along with the name of the subdirectory. In
contrast to Windows, there is no need for looking up the driver file path in a
COM class registry, it is contained in the ID string directly. The
`cwASIOload()` function used to load the driver takes advantage of this, and
uses the ID string directly to load the driver.

In the same way as on Windows, the driver instance needs to be initialized with
a call to its `init()` method, at which point it checks and initializes the
hardware.

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

## Writing a driver

cwASIO includes two code skeletons that you can use as a starting point for your
own driver implementation. One is for a driver written in C, and the other for
C++. There are numerous places in the skeletons where you are supposed to insert
your own code. They are marked with a brief comment.

The skeletons are supported by some elementary scaffolding for dealing with
system specifics like complying with the rules for dynamically loadable modules,
and their registration in the system, to support portability of your code. This
support code is contained in `cwASIOdriver.h` and `cwASIOdriver.c` files.

A driver is a shared library suitable for being loaded at runtime by a host
application. For the host application to be able to find it on the host system,
it must be registered, which is a process that depends on the OS used. The
information necessary for this registration process is a few GUIDs and some
texts that identify and describe the driver. You must fill in those values that
are specific for your driver.

### About GUIDs

On Windows, ASIO has always used GUIDs to unambiguously identify different
drivers. This is part of the Microsoft Component Object Model (COM), which
stipulates that both classes and interfaces are identified with a unique GUID,
which is also used for discovery via the Microsoft Windows Registry. We use them
in cwASIO forLinux, too, to a certain extent, in order to maintain a level of
similarity between the systems.

A GUID is a 128-bit data structure that is defined and described in RFC 9562, or
equivalently in ITU-T Rec. X.667. There is a binary representation and a textual
representation. The binary representation is using a big-endian format when
exchanged between systems, but the in-memory representation within a computer
may use other representations (and on little-endian systems that is often the
case). The textual representation uses hex digits and suffers from ambiguities
due to upper and lowercase letters being both allowed. The recommended
representation uses lowercase, but uppercase or mixed case must also be
accepted. We use the 8-4-4-4-12 format with braces, the Microsoft standard.

The use of a big-endian binary representation means that the order of the hex
digits in the textual representation matches the order in a hexdump of the
binary representation. For an in-memory representation, that doesn't necessarily
apply. For example, the type `cwASIOGUID` on a little-endian system will show a
different in-memory byte ordering than what the official binary representation
stipulates.

In cwASIO, we generate the textual representation on demand, and use the binary
representation internally. The generated textual representation uses lowercase
hex digits. When reading a textual representation, both cases are accepted. In
the Windows Registry, key comparison is case insensitive, hence a GUID can be
used in its textual representation as a Registry key without problems. When
doing your own comparisons, however, be aware of this problem.

In ASIO, the GUID that serves as the class ID to locate the driver on the
system, does double duty as the interface ID when creating a driver instance.
Those would normally be distinct GUIDs, but ASIO chose to simplify things. *(In
theory, a class and an interface are not the same thing. A class may implement
several different interfaces at the same time, and the interface ID would be
used to distinguish between them. This can be useful both for cases when
different versions of an interface exist, or when completely different
interfaces with different functionality are implemented)*.

The bottomline is that a driver provider must generate a unique GUID for the
driver, which is built into the driver code. This GUID is used in registering
the driver with your system, usually as part of the driver installation process,
and in creating a driver instance for use by an audio application.
