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

### Linking to cwASIO

cwASIO uses the CMake library type "OBJECT", which keeps the object files that
are built separate, instead of collecting them into a library file. When your
project uses CMake, you should use FetchContent to incorporate cwASIO into your
build, and then list the cwASIO targets you need among the dependent libraries.
If you are using other build tools, please include the cwASIO sources into your
build.

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

## Multi-instance issues

The original ASIO API was designed around the idea that an application can only
load a single driver, which is controlling a single device, and when one
application uses the device another application can't. This shows in various
places, and is difficult to overcome. However, with some careful thought and
implementation, all three restrictions can be overcome.

Overcoming the first one is called multi-driver support, which permits
applications to work with several drivers/devices at the same time, which has so
far been very rare in applications. Overcoming the second one is called
multi-instance support, which means that the same driver can be instantiated
several times for different devices. This doesn't appear to have been supported
until now at all. The third one is called multi-client support, which has been
offered for a long time by drivers of some manufacturers, notably MOTU and RME.

### Multidriver applications

This is the ability of an application to open more than one ASIO driver
simultaneously. ASIO can support this, but not with the standard C API. The
native API (in cwASIO represented by cwASIODriverVtbl) needs to be used
directly.

The callbacks of the driver API present an additional difficulty here. They lack
a context parameter that would permit to distinguish between the drivers, so the
application must use some trickery to identify the driver that called one of the
callbacks.

When using the native cwASIO API, an application can relatively easily support
multiple driver instances concurrently. Bear in mind, however, what this means
in practice: The application gets callback calls from multiple threads
concurrently, at possibly different rates. It depends on the application whether
that makes sense, and under what circumstances. Even if it does make sense, the
application needs to be written such that this level of concurrency doesn't
cause problems.

### Multiinstance drivers

This is the ability of a single driver to be instantiated multiple times for
different audio hardware. For example, think about two devices that are managed
by the same driver connected to the same computer, like two cards plugged in, or
two USB devices connected. This can't be done with standard ASIO. The workaround
used by some manufacturers is to load the driver once, and it takes care of all
the connected hardware devices, effectively making them look as if they were one
combined device. This only makes sense when the devices are synchronized between
each other, i.e. they work from a common clock, and share the same settings.

Loading the same driver separately for each of the devices, with individual
settings for them, is hampered by the difficulty to enumerate and identify the
different instances of the same driver. In theory, on Windows, there could be
multiple entries under the `HKEY_LOCAL_MACHINE\SOFTWARE\ASIO` registry key, one
for each hardware device, all using the same CLSID that leads to the same
driver, i.e. to the same entry under the `HKEY_CLASSES_ROOT\CLSID` registry key.
But the driver would need to know which of the several hardware devices it is
supposed to take care of. Some additional information would need to be passed to
the driver instance for allowing it to make that distinction, but ASIO doesn't
define a mechanism for doing that.

An extension to ASIO is defined by cwASIO to make this possible:

The `future()` method of the driver implements a new selector that allows
passing the name of the subkey in `HKEY_LOCAL_MACHINE\SOFTWARE\ASIO` mentioned
above. The call would need to be made before the call to `init()`, which is
implemented for the standard ASIO C API when you use the `ASIOLoad()` function
of cwASIO. A driver that doesn't implement this, which includes all legacy
drivers, would not understand this call and return an error code of
`ASE_InvalidParameter`. `ASIOLoad()` treats this as success, as the driver has been
successfully loaded, it just doesn't doesn't offer multiinstance support.

A driver that understands the new selector would store the name passed. The
subsequent `init()` call would use the stored name to distinguish between
different hardware devices, and to locate any further information specific to
the hardware device in the registry. If no `future()` call was made before
calling `init()`, no name is known, and the driver has to use a default name.

Calling `future()` with the new selector `kcwASIOsetInstanceName` will be
answered with `ASE_InvalidParameter` by a driver that offers no multiinstance
support. A driver that offers this support should verify that an entry with the
given name is present in the registry, in which case it returns `ASE_SUCCESS`.
If no matching registry entry was found, the return value is `ASE_NotPresent`.
Note that only the presence of the registry entry should be checked, not the
presence of the hardware device itself. Providing an empty name string (or NULL)
to the `future()` call should have no effect and return `ASE_SUCCESS` if the
driver offers multiinstance support. This may be used to check for multiinstance
support without setting a name.

For supporting legacy applications under Windows, there is also the possibility
of registering the same driver under several different CLSID values, which get
passed to the driver on instantiation. This can be used to select a different
default name for each CLSID passed.

Note that there are four combinations involving host applications and drivers
that may or may not support multiinstance:

1. Legacy application uses legacy driver (on Windows only).\
   Only a single instance of the driver can be used, and only one entry in the
   registry can be created for the driver. That entry will have the default name
   of the driver, which can't be changed, because the driver won't understand
   the `future()` call to change it. The legacy application won't set the name
   with `future()` anyway.
2. Legacy application uses cwASIO driver (on Windows only).\
   The driver may control more than one device, and there is a registry entry
   for each. The legacy application can enumerate them in the normal way. The
   legacy application won't use `future()` to set the instance name, so the
   driver will have to use a different default name for each instance. The way
   to do that is by using different CLSID entries in each registry entry, i.e.
   to register in COM the same driver DLL under several different CLSID values.
   The driver needs to have a built-in table that maps those CLSID values to
   default names. It would be up to the driver manufacturer to come up with a
   fixed list of CLSIDs for as many parallel instances as the driver is supposed
   to support. On driver instantiation, the CLSID that the application has
   chosen is passed to the queryInterface() function of the driver as the IID,
   so the driver has a chance to choose the appropriate default name based on
   the passed GUID.
3. cwASIO application uses legacy driver (on Windows only).\
   The driver will only offer one device, i.e. one registry entry, and it won't
   support the setting of an instance name through `future()`. The application,
   which will try to set the name with a call to `future()`, will receive an
   error, telling it that it is working with a legacy driver.
4. cwASIO application uses cwASIO driver (on all platforms).\
   There can be multiple registry entries for one driver, corresponding to
   multiple devices. The application instantiates one, and then sets its name
   with `future()`. On Windows, the driver would have chosen the appropriate
   default name via the IID already, so the call to `future()` would not change
   anything, except telling the driver that the application is not a legacy
   application (if that's something the driver would wish to know). On Linux, no
   IID is used, and the application can't be legacy, as this doesn't exist on
   Linux. In fact, on Linux, the call to `future()` to set the instance name is
   mandatory before calling `init()`.

### Multiclient drivers

This is the ability of a driver to accommodate several concurrent host
applications. ASIO does support this, and drivers can be readily written that
work in this way.

For the driver writer, this means that the driver module (DLL or shared object)
can be loaded concurrently by several processes, each of which will call
`init()` on its interface. Since the hardware only exists once, the driver will
have to keep track of the number of applications using it, and only initialize
the hardware when the first application calls `init()`, and deinitialize it when
the last one disappears. It will also have to manage conflicts when applications
want to set the sampling rate, or other parameters, that would affect all host
applications.

Typically, the clients will use different channels of the same device, in order
not to compromise the audio of other clients. In most cases, the driver doesn't
attempt to mix the audio from different clients, although that would be
technically feasible, with some potential loss of efficiency.

It goes without saying that access to the common hardware will have to be
protected from concurrent access by different applications.

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
it must be registered, which is a process that depends on the OS used. You must
provide the information necessary for this registration process by initializing
a few global constants.

### Windows specifics, including the use of GUIDs

On Windows, ASIO has always used GUIDs to unambiguously identify different
drivers. This is part of the Microsoft Component Object Model (COM), which
stipulates that both classes and interfaces are identified with a unique GUID,
which is also used for discovery via the Microsoft Windows Registry. Under
Linux, this functionality doesn't exist, and we have to come up with a scheme
that identifies drivers in a similar way.

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
system, does double duty as the interface ID upon creating a driver instance.
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

The possibility of multiinstance drivers creates an additional challenge here.
Compatibility with Windows legacy host applications demands that they be
registered with COM several times under distinct GUIDs. Each instance is
responsible for a different audio device. We have more to say about this later.

## Installing the driver

Once your driver is built, it is contained in a DLL (Windows) or a shared object
file (Linux). For a host application to be able to find and load it, it must be
registered with the system. The driver contains functions you can call as part
of the installation process, to create the necessary entries in the system
registry. The process is different between Linux and Windows, and the functions
to call are also different. This is appropriate, as you will also need to
provide different installers for Linux and Windows.

Creating installers for the systems you want to support is up to you. There is
only minimal support in cwASIO, in the form of registration/unregistration
functions.

In general, the process is to put the driver file into the place where you want
it to be, in the system's directory tree, and then to load the driver and call
its registration function. Then unload the driver again.

Uninstalling is the process in reverse; you load the driver, call its
unregistration function, unload the driver, and then clean up by deleting the
driver file, and anything else that might need deleting.

### Installing on Windows

On Windows, you will most probably want to install the driver into a
subdirectory of the "Program Files" directory, but you would typically provide
the user with an opportunity to change that location. Wherever the driver ends
up being placed, you need to first put it there, and then load it and call its
`DllRegisterServer` function. You would not normally do this yourself, but you
would use the `Regsrv32` utility to do this for you. This is a command line
utility that your installer would call to do the registration.

Uninstalling uses the same utility, which calls the `DllUnregisterServer`
function of the driver. After that, you would delete the driver file and the
directory you created.

There are two versions of the `Regsrv32` utility, a 64-bit version and a 32-bit
version. The have the same name, but are located in different places in the file
system, so don't let the file name mislead you. To register a 64-bit driver, you
need to use the 64-bit version of `Regsrv32`. Likewise, to register a 32-bit
version of the driver, you need to use the 32-bit version of `Regsrv32` utility.

For the details, please refer to the documentation of `Regsrv32` by Microsoft.

Calling `DllRegisterServer()` doesn't allow passing any parameters to it, which
is OK when registering a driver that only supports one device. A multiinstance
driver, however, needs to be installed several times, once for each device, with
the driver DLL being common. In this case, the `DllRegisterServer` function
needs to be able to tell which device is being installed, and likewise the
`DllUnregisterServer` needs to know which one is being uninstalled. The way to
tell those functions is via an environment variable named "CWASIO_INSTALL_NAME",
which only needs to be set temporarily while the installer is running. The
environment variable needs to be set to contain the name of the device to
install or uninstall. The driver uses this in its `DllRegisterServer` and
`DllUnregisterServer` functions to determine the registry entry it needs to act
upon. If the environment variable is missing, it acts on the first (or only)
device name it knows. Hence, the installer would set the environment variable
before calling `DllRegisterServer` or `DllUnregisterServer`, and delete it after
they return.

Of course, the installer may add additional information to the registry entry
that `DllRegisterServer` created. Most importantly, this would be the
description entry that contains the text that would typically be shown to a user
who wants to select a device from a list of available devices. While the name
would be chosen from a hardcoded list defined by the driver, the description
would be provided by the installer, possibly obtained from the device itself, or
from user input.

### Installing on Linux

The installation location of the driver on Linux will probably depend on the
distribution you address. We don't prescribe anything here, you make your own
choices. Otherwise the process involves calling the function `registerDriver`
during installation, and `unregisterDriver` during deinstallation. Those two
functions take care of maintaining the entry in `/etc/cwASIO`. It is the job of
your installer to take care of copying/deleting the actual driver file, and any
further files and directories your driver might need.

On Linux, there is no utility comparable with `Regsrv32` on Windows, so you need
to call the functions mentioned above by yourself.

Note that `unregisterDriver` can't delete the subdirectory that `registerDriver`
created, unless it is empty after deleting the files `driver` and `description`.
Your installer or driver may create additional files in there, but when
uninstalling, they should be deleted before calling `unregisterDriver`, except
if you deliberately want to keep the directory. Both functions allow passing the
registration name, so there is no need for setting an environment variable as
under Windows in the case of multiinstance drivers.

Of course, you must have the right to write to `/etc/cwASIO`, otherwise the
calls to `registerDriver` or `unregisterDriver` will fail with an error
indicating insufficient rights. Both functions return 0 on success, and an errno
in case of failure.

## Handling driver settings

A driver will likely have to store device specific settings, and read them when
starting up. In case of a multiinstance driver, the location will differ between
different devices the driver controls. A driver that supports only one instance
will use a single, fixed location. See the description above of multiinstance
drivers.

It is up to the driver writer to define where those settings are stored. There
is nothing ASIO or cwASIO defines here. Your driver may want to store settings
per user, per hardware configuration, per device or per driver, or any
combination thereof. The driver implements this all by itself, with no
particular cwASIO support.

However, a few things must be borne in mind here in conjunction with cwASIO. The
most intuitive location for storing device-specific settings is in the same
place where the devices are listed for enumeration, i.e. in the Windows
Registry, or in `/etc/cwASIO` on Linux, which is why there is support for it
with the function `cwASIOgetParameter()`, defined in `cwASIO.h`, which can be
used to retrieve entries from the same place where the driver is registered.
They would have been placed there on installation, and the function provides an
easy way for an application or driver to retrieve them. For user specific
settings, there is the possibility to store them in files in the user's home
directory, where the appropriate access rights are in force, or (on Windows) in
a registry place where the user has write access. For those there's no direct
cwASIO support.

If the driver doesn't support multiinstance, it must have the name hardcoded,
under which the entry in the registry is found (we call it the default name). A
multiinstance driver permits overriding the default name with the `future()`
call described above. This means that reading the settings should be done by the
`init()` function, because only then the driver instance knows its name, which
it will need to locate the proper settings. The `init()` function also has the
ability to report any errors that were encountered while reading the settings,
and applying them to the device.

The prerequisite for this to work is that the driver instance uses the same name
under which the device is listed in the registry. Thus, the host application
must pass the exact name to the driver with the `future()` call, under which it
found the driver during enumeration. The driver should check in the `future()`
call if the registry contains an entry for this name, but it should leave the
reading of the settings to the `init()` function.

The driver should report back in `getDriverName()` the same name that was set
with the `future()` call, if that was successful. Otherwise it should report
back the default name.

To see an example how the host application is supposed to handle this, refer to
`test/application.c`.
