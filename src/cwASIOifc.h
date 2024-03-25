/** @file       cwASIOifc.h
 *  @brief      cwASIO driver API (Unix)
 *  @author     Axel Holzinger
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2014-2024
 *  @copyright  Usage and copying requires a license granted by the authors
 * @addtogroup AsioDevice
 *  @{
 */
#pragma once

#include "cwASIO.h"
#include <stdbool.h>


struct AsioDriver;

struct cwASIO_DriverInterface {
    void *driverLib;
    struct AsioDriver *(*instantiate)(void *);
    void (*discard)(struct AsioDriver *);
    void (*getDriverName)(struct AsioDriver *, char *);
    long (*getDriverVersion)(struct AsioDriver *);
    void (*getErrorMessage)(struct AsioDriver *, char *);
    long (*start)(struct AsioDriver *);
    long (*stop)(struct AsioDriver *);
    long (*getChannels)(struct AsioDriver *, long *, long *);
    long (*getLatencies)(struct AsioDriver *, long *, long *);
    long (*getBufferSize)(struct AsioDriver *, long *, long *, long *, long *);
    long (*canSampleRate)(struct AsioDriver *, double);
    long (*getSampleRate)(struct AsioDriver *, double *);
    long (*setSampleRate)(struct AsioDriver *, double);
    long (*getClockSources)(struct AsioDriver *, ASIOClockSource *, long *);
    long (*setClockSource)(struct AsioDriver *, long);
    long (*getSamplePosition)(struct AsioDriver *, ASIOSamples *, ASIOTimeStamp *);
    long (*getChannelInfo)(struct AsioDriver *, ASIOChannelInfo *);
    long (*createBuffers)(struct AsioDriver *, ASIOBufferInfo *, long , long , ASIOCallbacks const *);
    long (*disposeBuffers)(struct AsioDriver *);
    long (*controlPanel)(struct AsioDriver *);
    long (*future)(struct AsioDriver *, long, void *);
    long (*outputReady)(struct AsioDriver *);
};

typedef bool (EnumerateCallback)(void *, char const *, char const *, char const *);

int cwASIOenumerate(EnumerateCallback *cb, void *context);

long cwASIOload(char const *path, struct cwASIO_DriverInterface *ifc);

void cwASIOunload(struct cwASIO_DriverInterface *ifc);

/** @}*/
