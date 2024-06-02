/** @file       cwASIOifc.h
 *  @brief      cwASIO driver API
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */
#pragma once

#include "cwASIO.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#   include <guiddef.h>
#   define CWASIO_METHOD __stdcall     // only relevant in 32-bit Windows
#else
#   define CWASIO_METHOD

struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
};
#endif

typedef struct _GUID cwASIOGUID;
struct cwAsioDriver;

struct cwASIO_DriverVtbl {
    long (CWASIO_METHOD *QueryInterface)(struct cwAsioDriver *, cwASIOGUID const *, void **);
    unsigned long (CWASIO_METHOD *AddRef)(struct cwAsioDriver *);
    unsigned long (CWASIO_METHOD *Release)(struct cwAsioDriver *);
    cwASIOBool (CWASIO_METHOD *init)(struct cwAsioDriver *, void *);
    void (CWASIO_METHOD *getDriverName)(struct cwAsioDriver *, char *);
    long (CWASIO_METHOD *getDriverVersion)(struct cwAsioDriver *);
    void (CWASIO_METHOD *getErrorMessage)(struct cwAsioDriver *, char *);
    cwASIOError (CWASIO_METHOD *start)(struct cwAsioDriver *);
    cwASIOError (CWASIO_METHOD *stop)(struct cwAsioDriver *);
    cwASIOError (CWASIO_METHOD *getChannels)(struct cwAsioDriver *, long *, long *);
    cwASIOError (CWASIO_METHOD *getLatencies)(struct cwAsioDriver *, long *, long *);
    cwASIOError (CWASIO_METHOD *getBufferSize)(struct cwAsioDriver *, long *, long *, long *, long *);
    cwASIOError (CWASIO_METHOD *canSampleRate)(struct cwAsioDriver *, double);
    cwASIOError (CWASIO_METHOD *getSampleRate)(struct cwAsioDriver *, double *);
    cwASIOError (CWASIO_METHOD *setSampleRate)(struct cwAsioDriver *, double);
    cwASIOError (CWASIO_METHOD *getClockSources)(struct cwAsioDriver *, struct cwASIOClockSource *, long *);
    cwASIOError (CWASIO_METHOD *setClockSource)(struct cwAsioDriver *, long);
    cwASIOError (CWASIO_METHOD *getSamplePosition)(struct cwAsioDriver *, cwASIOSamples *, cwASIOTimeStamp *);
    cwASIOError (CWASIO_METHOD *getChannelInfo)(struct cwAsioDriver *, struct cwASIOChannelInfo *);
    cwASIOError (CWASIO_METHOD *createBuffers)(struct cwAsioDriver *, struct cwASIOBufferInfo *, long , long , struct cwASIOCallbacks const *);
    cwASIOError (CWASIO_METHOD *disposeBuffers)(struct cwAsioDriver *);
    cwASIOError (CWASIO_METHOD *controlPanel)(struct cwAsioDriver *);
    cwASIOError (CWASIO_METHOD *future)(struct cwAsioDriver *, long, void *);
    cwASIOError (CWASIO_METHOD *outputReady)(struct cwAsioDriver *);
};

struct cwAsioDriver {
    struct cwASIO_DriverVtbl const *vtbl;
};

typedef bool (cwASIOcallback)(void*, char const*, char const*, char const*);

/** Enumerate devices installed on the system.
* An enumeration function is passed to cwASIOenumerate(), which will get called once for every entry found in the ASIO driver list.
* If the enumeration function returns with a true result, enumeration continues with the next entry, otherwise enumeration terminates.
* Three strings are passed to the enumeration function: Name, Path and Description. The latter two might be empty, if the
* corresponding registry entry is empty or absent. You might want to ignore such entries.
* @param cb Pointer to callback function
* @param context Pointer to be forwarded to callback function as its first parameter.
* @return error code, which is zero on success.
*/
int cwASIOenumerate(cwASIOcallback *cb, void *context);

/** Load the ASIO driver.
* @param id On Windows: the CLSID, on Linux: the file path of the driver to load.
* @param drv Receives a pointer to the driver instance.
* @return an error code when unsuccessful, zero on success.
*/
long cwASIOload(char const *key, struct cwAsioDriver **drv);

/** Unload the ASIO driver.
* @param drv Pointer to the driver instance that was initialized by cwASIOload()
*/
void cwASIOunload(struct cwAsioDriver *drv);

/** @}*/
