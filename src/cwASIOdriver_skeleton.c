/** @file       cwASIOdriver_skeleton.c
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 */
#pragma once

#include "cwASIOdriver.h"
#include <stdatomic.h>
#include <stddef.h>
// ... (add here any further includes you may need)


/** Your driver implemented as a C struct. */
struct MyAsioDriver {
    struct cwASIODriver base;   // must be the first struct member
    atomic_ulong references;    // threadsafe reference counter
    char name[33];              // the name of this instance
    // ... (more data members here)
};

static long CWASIO_METHOD queryInterface(struct cwASIODriver *drv, cwASIOGUID const *guid, void **ptr) {
    struct MyAsioDriver *self = drv;
    long res = cwASIOfindName(guid, self->name, 32);
    if(res > 0)
        self->name[32] = '\0';  // ensure null termination
    if(res < 0)
        return -res;            // GUID not found in registry
    *ptr = drv;
    drv->lpVtbl->addRef(drv);
    return 0;                   // success
}

static unsigned long CWASIO_METHOD addRef(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    return atomic_fetch_add(&self->references, 1) + 1;
}

static unsigned long CWASIO_METHOD release(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    unsigned long res = atomic_fetch_sub(&self->references, 1) - 1;
    if (res == 0) {
        free(drv);
        atomic_fetch_sub(&activeInstances, 1);
    }
    return res;
}

static cwASIOBool CWASIO_METHOD init(struct cwASIODriver *drv, void *sys) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    if(!self || !self->name[0])
        return ASIOFalse;
    // ... (do the driver initialization here)
    return ASIOTrue;
}

static void CWASIO_METHOD getDriverName(struct cwASIODriver *drv, char *buf) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    if (self && self->name[0] && buf)
        strcpy(buf, self->name);
}

static long CWASIO_METHOD getDriverVersion(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return 0;
}

static void CWASIO_METHOD getErrorMessage(struct cwASIODriver *drv, char *buf) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
}

static cwASIOError CWASIO_METHOD start(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD stop(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getChannels(struct cwASIODriver *drv, long *in, long *out) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getLatencies(struct cwASIODriver *drv, long *in, long *out) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getBufferSize(struct cwASIODriver *drv, long *min, long *max, long *pref, long *gran) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD canSampleRate(struct cwASIODriver *drv, double srate) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getSampleRate(struct cwASIODriver *drv, double *srate) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD setSampleRate(struct cwASIODriver *drv, double srate) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getClockSources(struct cwASIODriver *drv, struct cwASIOClockSource *clocks, long *num) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD setClockSource(struct cwASIODriver *drv, long ref) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getSamplePosition(struct cwASIODriver *drv, cwASIOSamples *sPos, cwASIOTimeStamp *tStamp) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD getChannelInfo(struct cwASIODriver *drv, struct cwASIOChannelInfo *info) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD createBuffers(struct cwASIODriver *drv, struct cwASIOBufferInfo *infos, long num, long size, struct cwASIOCallbacks const *cb) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD disposeBuffers(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD controlPanel(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD future(struct cwASIODriver *drv, long sel, void *par) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    switch (sel) {
    // ... (insert code for your other cases here)
    case kcwASIOsetInstanceName:
        if (!par || *(char const *)par == '\0')
            return ASE_SUCCESS;
        if (strlen((char const *)par) > 32)
            return ASE_NotPresent;
        if (0 == cwASIOgetParameter((char const *)par, NULL, NULL, 0)) {
            strncpy(self->name, (char const *)par, 32);
            return ASE_SUCCESS;
        }
        return ASE_NotPresent;
    default:
        return ASE_InvalidParameter;
    }
    return ASE_OK;
}

static cwASIOError CWASIO_METHOD outputReady(struct cwASIODriver *drv) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    // ... (insert your code here)
    return ASE_OK;
}

struct cwASIODriverVtbl const myAsioDriverVtbl = {
    .queryInterface = &queryInterface,
    .addRef = &addRef,
    .release = &release,
    .init = &init,
    .getDriverName = &getDriverName,
    .getDriverVersion = &getDriverVersion,
    .getErrorMessage = &getErrorMessage,
    .start = &start,
    .stop = &stop,
    .getChannels = &getChannels,
    .getLatencies = &getLatencies,
    .getBufferSize = &getBufferSize,
    .canSampleRate = &canSampleRate,
    .getSampleRate = &getSampleRate,
    .setSampleRate = &setSampleRate,
    .getClockSources = &getClockSources,
    .setClockSource = &setClockSource,
    .getSamplePosition = &getSamplePosition,
    .getChannelInfo = &getChannelInfo,
    .createBuffers = &createBuffers,
    .disposeBuffers = &disposeBuffers,
    .controlPanel = &controlPanel,
    .future = &future,
    .outputReady = &outputReady
};

struct cwASIODriver *makeAsioDriver() {
    struct MyAsioDriver *obj = malloc(sizeof(struct MyAsioDriver));
    if(!obj)
        return NULL;     // lack of sufficient memory
    obj->base.lpVtbl = &myAsioDriverVtbl;
    atomic_init(&obj->references, 1);
    obj->name[0] = '\0';    // no name yet
    // .... (you may do some more member initialization here)
    return &obj->base;
}
