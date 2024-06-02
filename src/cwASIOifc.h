/** @file       cwASIOifc.h
 *  @brief      cwASIO driver API (Unix)
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
#   define CWASIO_METHOD __stdcall     // only relevant in 32-bit Windows
#else
#   define CWASIO_METHOD
#endif

struct AsioDriver;

struct GUID {
    unsigned long  data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char  data4[8];
};

struct cwASIO_DriverVtbl {
    long (CWASIO_METHOD *QueryInterface)(struct AsioDriver *, struct GUID const *, void **);
    unsigned long (CWASIO_METHOD *AddRef)(struct AsioDriver *);
    unsigned long (CWASIO_METHOD *Release)(struct AsioDriver *);
    cwASIOBool (CWASIO_METHOD *init)(struct AsioDriver*, void *);
    void (CWASIO_METHOD *getDriverName)(struct AsioDriver *, char *);
    long (CWASIO_METHOD *getDriverVersion)(struct AsioDriver *);
    void (CWASIO_METHOD *getErrorMessage)(struct AsioDriver *, char *);
    cwASIOError (CWASIO_METHOD *start)(struct AsioDriver *);
    cwASIOError (CWASIO_METHOD *stop)(struct AsioDriver *);
    cwASIOError (CWASIO_METHOD *getChannels)(struct AsioDriver *, long *, long *);
    cwASIOError (CWASIO_METHOD *getLatencies)(struct AsioDriver *, long *, long *);
    cwASIOError (CWASIO_METHOD *getBufferSize)(struct AsioDriver *, long *, long *, long *, long *);
    cwASIOError (CWASIO_METHOD *canSampleRate)(struct AsioDriver *, double);
    cwASIOError (CWASIO_METHOD *getSampleRate)(struct AsioDriver *, double *);
    cwASIOError (CWASIO_METHOD *setSampleRate)(struct AsioDriver *, double);
    cwASIOError (CWASIO_METHOD *getClockSources)(struct AsioDriver *, struct cwASIOClockSource *, long *);
    cwASIOError (CWASIO_METHOD *setClockSource)(struct AsioDriver *, long);
    cwASIOError (CWASIO_METHOD *getSamplePosition)(struct AsioDriver *, cwASIOSamples *, cwASIOTimeStamp *);
    cwASIOError (CWASIO_METHOD *getChannelInfo)(struct AsioDriver *, struct cwASIOChannelInfo *);
    cwASIOError (CWASIO_METHOD *createBuffers)(struct AsioDriver *, struct cwASIOBufferInfo *, long , long , struct cwASIOCallbacks const *);
    cwASIOError (CWASIO_METHOD *disposeBuffers)(struct AsioDriver *);
    cwASIOError (CWASIO_METHOD *controlPanel)(struct AsioDriver *);
    cwASIOError (CWASIO_METHOD *future)(struct AsioDriver *, long, void *);
    cwASIOError (CWASIO_METHOD *outputReady)(struct AsioDriver *);
};

struct AsioDriver {
    struct cwASIO_DriverVtbl const *vtbl;
};

typedef bool (cwASIOcallback)(void*, char const*, char const*, char const*);

/** Enumerate devices from /etc/cwASIO.
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
* @param key On Linux, the file path of the driver to load.
* @param drv Receives a pointer to the driver instance.
* @return an error code when unsuccessful, zero on success.
*/
long cwASIOload(char const *key, struct AsioDriver **drv);

/** Unload the ASIO driver.
* @param ifc Pointer to the driver interface that was initialized by cwASIOload()
*/
void cwASIOunload(struct AsioDriver *ifc);

/** @}*/
