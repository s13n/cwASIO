/** @file       cwASIOdriver_skeleton.c
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 */
#pragma once

#include "cwASIOdriver.h"
#include <stdatomic.h>
#include <stddef.h>
// ... (add here any further includes you may need)


// Initialize the following table with the values for your driver.
// Note that the names are limited to a maximum length of 32 characters including the terminating nullbyte.
struct cwASIOinstance const instanceTable[] = {
    { .name = "Instance #1", .guid = {0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    // ... more instances can follow here, each with their own name and GUID
    { .name = NULL }        // this terminates the list and must always be there.
};


/** Your driver implemented as a C struct. */
struct MyAsioDriver {
    struct cwASIODriver base;   // must be the first struct member
    atomic_ulong references;    // threadsafe reference counter
    struct cwASIOinstance const *instance;  // which instance was selected
    // ... (more data members here)
};

static long CWASIO_METHOD queryInterface(struct cwASIODriver *drv, cwASIOGUID const *guid, void **ptr) {
    if (guid) {     // This is only non-null on Windows
        long err = E_NOINTERFACE;
        // find guid in our instance table
        for (struct cwASIOinstance const *entry = instanceTable; entry->name; ++entry) {
            if (cwASIOcompareGUID(guid, &entry->guid)) {
                struct MyAsioDriver *self = drv;
                self->instance = entry;
                err = 0;     // success
                break;
            }
        }
        if (err) {
            *ptr = NULL;
            return err;     // none of our guids
        }
    }
    *ptr = drv;
    drv->lpVtbl->addRef(drv);
    return 0;       // success
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
    if(/* already initialized */ 0)
        return ASIOFalse;
    // ... (do the driver initialization here)
    return ASIOTrue;
}

static void CWASIO_METHOD getDriverName(struct cwASIODriver *drv, char *buf) {
    struct MyAsioDriver *self = (struct MyAsioDriver*)drv;
    if (buf)
        strcpy(buf, self->instance->name);
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
        for (struct cwASIOinstance const *entry = instanceTable; entry->name; ++entry) {
            if (0 == strcmp((char const *)par, entry->name)) {
                self->instance = entry;
                if (0 == cwASIOgetParameter(entry->name, NULL, NULL, 0))
                    return ASE_SUCCESS;
                break;
            }
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
    obj->instance = instanceTable;      // first entry
    // .... (you may do some more member initialization here)
    return &obj->base;
}
