/** @file       cwASIO.h
 *  @brief      cwASIO native API
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */
#pragma once

#include "cwASIOtypes.h"
#include <stdbool.h>

#ifdef _WIN32
#   define CWASIO_METHOD __stdcall     // only relevant in 32-bit Windows
#else
#   define CWASIO_METHOD
#endif

typedef struct _GUID cwASIOGUID;
struct cwASIODriver;

struct cwASIODriverVtbl {
    long (CWASIO_METHOD *QueryInterface)(struct cwASIODriver *, cwASIOGUID const *, void **);
    unsigned long (CWASIO_METHOD *AddRef)(struct cwASIODriver *);
    unsigned long (CWASIO_METHOD *Release)(struct cwASIODriver *);
    cwASIOBool (CWASIO_METHOD *init)(struct cwASIODriver *, void *);
    void (CWASIO_METHOD *getDriverName)(struct cwASIODriver *, char *);
    long (CWASIO_METHOD *getDriverVersion)(struct cwASIODriver *);
    void (CWASIO_METHOD *getErrorMessage)(struct cwASIODriver *, char *);
    cwASIOError (CWASIO_METHOD *start)(struct cwASIODriver *);
    cwASIOError (CWASIO_METHOD *stop)(struct cwASIODriver *);
    cwASIOError (CWASIO_METHOD *getChannels)(struct cwASIODriver *, long *, long *);
    cwASIOError (CWASIO_METHOD *getLatencies)(struct cwASIODriver *, long *, long *);
    cwASIOError (CWASIO_METHOD *getBufferSize)(struct cwASIODriver *, long *, long *, long *, long *);
    cwASIOError (CWASIO_METHOD *canSampleRate)(struct cwASIODriver *, double);
    cwASIOError (CWASIO_METHOD *getSampleRate)(struct cwASIODriver *, double *);
    cwASIOError (CWASIO_METHOD *setSampleRate)(struct cwASIODriver *, double);
    cwASIOError (CWASIO_METHOD *getClockSources)(struct cwASIODriver *, struct cwASIOClockSource *, long *);
    cwASIOError (CWASIO_METHOD *setClockSource)(struct cwASIODriver *, long);
    cwASIOError (CWASIO_METHOD *getSamplePosition)(struct cwASIODriver *, cwASIOSamples *, cwASIOTimeStamp *);
    cwASIOError (CWASIO_METHOD *getChannelInfo)(struct cwASIODriver *, struct cwASIOChannelInfo *);
    cwASIOError (CWASIO_METHOD *createBuffers)(struct cwASIODriver *, struct cwASIOBufferInfo *, long , long , struct cwASIOCallbacks const *);
    cwASIOError (CWASIO_METHOD *disposeBuffers)(struct cwASIODriver *);
    cwASIOError (CWASIO_METHOD *controlPanel)(struct cwASIODriver *);
    cwASIOError (CWASIO_METHOD *future)(struct cwASIODriver *, long, void *);
    cwASIOError (CWASIO_METHOD *outputReady)(struct cwASIODriver *);
};

struct cwASIODriver {
    struct cwASIODriverVtbl const *lpVtbl;
};

typedef bool (cwASIOcallback)(void*, char const*, char const*, char const*);

/** Enumerate devices installed on the system.
 * An enumeration function is passed to `cwASIOenumerate()`, which will get called once
 * for every entry found in the installed drivers list. If the enumeration function
 * returns with a true result, enumeration continues with the next entry, otherwise
 * enumeration terminates. Three strings are passed to the enumeration function:
 * Name, Path and Description. The latter two might be empty, if the corresponding
 * drivers list entry is empty or absent. You might want to ignore such entries.
 * @param cb Pointer to callback function
 * @param context Pointer to be forwarded to callback function as its first parameter.
 * @return error code, which is zero on success.
 */
int cwASIOenumerate(cwASIOcallback *cb, void *context);

/** Load the driver.
 * @param id On Windows: the CLSID, on Linux: the file path of the driver to load.
 * @param drv Receives a pointer to the driver instance.
 * @return an error code when unsuccessful, zero on success.
 */
long cwASIOload(char const *key, struct cwASIODriver **drv);

/** Unload the driver.
 * @param drv Pointer to the driver instance that was initialized by cwASIOload()
 */
void cwASIOunload(struct cwASIODriver *drv);

/** Compare two GUIDs for equality.
 * @param a Pointer to first GUID
 * @param a Pointer to second GUID
 * @return true if a and b are the same, false otherwise.
 */
bool cwASIOcompareGUID(cwASIOGUID const *a, cwASIOGUID const *b);

/** Convert a class id in MS CLSID style to a GUID.
 * The CLSID must include the curly braces.
 * @param clsid The textual CLSID form.
 * @return the resulting GUID
 */
cwASIOGUID cwASIOtoGUID(char const *clsid);

/** @}*/
