/** @file       cwASIOdriver_skeleton.cpp
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 */

extern "C" {
    #include "cwASIOdriver.h"
}
#include <atomic>
#include <cstring>
#include <exception>
// ... (add here any further includes you may need)


// Initialize the following table with the values for your driver.
// Note that the names are limited to a maximum length of 32 characters including the terminating nullbyte.
struct cwASIOinstance const cwAsioDriverInstances[] = {
    { .name = "Instance #1", .guid = {0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    // ... more instances can follow here, each with their own name and GUID
    { .name = NULL }        // this terminates the list and must always be there.
};

std::atomic_uint activeInstances = 0;

/** Your driver implemented as a C++ class. */
class MyAsioDriver : public cwASIODriver {
    MyAsioDriver(MyAsioDriver &&) =delete;  // no move/copy

public:
    MyAsioDriver()
        : cwASIODriver{ &vtbl }
        , references{1}
        , instance{cwAsioDriverInstances}   // first entry
        // .... (you may do some more member initialization here)
    {
    }

    long queryInterface(cwASIOGUID const *guid, void **ptr) {
        if (guid) {     // This is only non-null on Windows
            long err = E_NOINTERFACE;
            // find guid in our instance table
            for (struct cwASIOinstance const *entry = cwAsioDriverInstances; entry->name; ++entry) {
                if (cwASIOcompareGUID(guid, &entry->guid)) {
                    instance = entry;
                    err = 0;     // success
                    break;
                }
            }
            if (err) {
                *ptr = NULL;
                return err;     // none of our guids
            }
        }
        // It's our GUID
        *ptr = this;
        addRef();
        return 0;       // success
    }

    unsigned long addRef() {
        return references.fetch_add(1) + 1;
    }

    unsigned long release() {
        unsigned long res = references.fetch_sub(1) - 1;
        if (res == 0) {
            delete this;
            atomic_fetch_sub(&activeInstances, 1);
        }
        return res;
    }

    cwASIOBool init(void *sys) {
        if(/* already initialized */ 0)
            return ASIOFalse;
        // ... (do the driver initialization here)
        return ASIOTrue;
    }

    void getDriverName(char *buf) {
        if (instance && buf)
            strcpy(buf, instance->name);
    }

    long getDriverVersion() {
        // ... (insert your code here)
        return 0;
    }

    void getErrorMessage(char *buf) {
        // ... (insert your code here)
    }

    cwASIOError start() {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError stop() {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getChannels(long *in, long *out) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getLatencies(long *in, long *out) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getBufferSize(long *min, long *max, long *pref, long *gran) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError canSampleRate(double srate) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getSampleRate(double *srate) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError setSampleRate(double srate) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getClockSources(struct cwASIOClockSource *clocks, long *num) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError setClockSource(long ref) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getSamplePosition(cwASIOSamples *sPos, cwASIOTimeStamp *tStamp) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError getChannelInfo(struct cwASIOChannelInfo *info) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError createBuffers(struct cwASIOBufferInfo *infos, long num, long size, struct cwASIOCallbacks const *cb) {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError disposeBuffers() {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError controlPanel() {
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError future(long sel, void *par) {
        switch (sel) {
        // ... (insert code for your other cases here)
        case kcwASIOsetInstanceName:
            for (struct cwASIOinstance const *entry = cwAsioDriverInstances; entry->name; ++entry) {
                if (0 == strcmp(static_cast<char const *>(par), entry->name)) {
                    if (0 != cwASIOgetParameter(entry->name, NULL, NULL, 0))
                        break;      // not registered
                    instance = entry;
                    return ASE_SUCCESS;
                }
            }
            return ASE_NotPresent;
        default:
            return ASE_InvalidParameter;
        }
        return ASE_OK;
    }

    cwASIOError outputReady() {
        // ... (insert your code here)
        return ASE_OK;
    }

private:
    static struct cwASIODriverVtbl const vtbl;

    std::atomic_ulong references;    // threadsafe reference counter
    struct cwASIOinstance const *instance;  // which instance was selected
    // ... (more data members here)
};

struct cwASIODriverVtbl const MyAsioDriver::vtbl = {
    [](cwASIODriver *drv, cwASIOGUID const *guid, void **ptr){ return static_cast<MyAsioDriver*>(drv)->queryInterface(guid, ptr); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->addRef(); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->release(); },
    [](cwASIODriver *drv, void *sys){ return static_cast<MyAsioDriver*>(drv)->init(sys); },
    [](cwASIODriver *drv, char *buf){ static_cast<MyAsioDriver*>(drv)->getDriverName(buf); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->getDriverVersion(); },
    [](cwASIODriver *drv, char *buf){ return static_cast<MyAsioDriver*>(drv)->getErrorMessage(buf); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->start(); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->stop(); },
    [](cwASIODriver *drv, long *in, long *out){ return static_cast<MyAsioDriver*>(drv)->getChannels(in, out); },
    [](cwASIODriver *drv, long *in, long *out){ return static_cast<MyAsioDriver*>(drv)->getLatencies(in, out); },
    [](cwASIODriver *drv, long *min, long *max, long *pref, long *gran){ return static_cast<MyAsioDriver*>(drv)->getBufferSize(min, max, pref, gran); },
    [](cwASIODriver *drv, double srate){ return static_cast<MyAsioDriver*>(drv)->canSampleRate(srate); },
    [](cwASIODriver *drv, double *srate){ return static_cast<MyAsioDriver*>(drv)->getSampleRate(srate); },
    [](cwASIODriver *drv, double srate){ return static_cast<MyAsioDriver*>(drv)->setSampleRate(srate); },
    [](cwASIODriver *drv, cwASIOClockSource *clocks, long *num){ return static_cast<MyAsioDriver*>(drv)->getClockSources(clocks, num); },
    [](cwASIODriver *drv, long ref){ return static_cast<MyAsioDriver*>(drv)->setClockSource(ref); },
    [](cwASIODriver *drv, cwASIOSamples *sPos, cwASIOTimeStamp *tStamp){ return static_cast<MyAsioDriver*>(drv)->getSamplePosition(sPos, tStamp); },
    [](cwASIODriver *drv, cwASIOChannelInfo *info){ return static_cast<MyAsioDriver*>(drv)->getChannelInfo(info); },
    [](cwASIODriver *drv, cwASIOBufferInfo *infos, long num, long size, cwASIOCallbacks const *cb){ return static_cast<MyAsioDriver*>(drv)->createBuffers(infos, num, size, cb); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->disposeBuffers(); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->controlPanel(); },
    [](cwASIODriver *drv, long sel, void *par){ return static_cast<MyAsioDriver*>(drv)->future(sel, par); },
    [](cwASIODriver *drv){ return static_cast<MyAsioDriver*>(drv)->outputReady(); }
};

cwASIODriver *makeAsioDriver() {
    try {
        return new MyAsioDriver();
    } catch(std::exception &ex) {
        return nullptr;
    }
}
