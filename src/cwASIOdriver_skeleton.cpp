/** @file       cwASIOdriver_skeleton.cpp
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 */
#pragma once

extern "C" {
    #include "cwASIOdriver.h"
}
#include <atomic>
#include <exception>
// ... (add here any further includes you may need)


// Initialize the following data constants with the values for your driver.
cwASIOGUID const cwAsioDriverCLSID = {/*0x________,0x____,0x____,0x__,0x__,0x__,0x__,0x__,0x__,0x__,0x__*/};
char const *cwAsioDriverKey = "";
char const *cwAsioDriverDescription = "";


/** Your driver implemented as a C++ class. */
class MyAsioDriver : public cwASIODriver {
    MyAsioDriver(MyAsioDriver &&) =delete;  // no move/copy

public:
    MyAsioDriver()
        : cwASIODriver{ &vtbl }
        , references{0}
        // .... (you may do some more member initialization here)
    {
    }

    long queryInterface(cwASIOGUID const *guid, void **ptr) {
        if (!cwASIOcompareGUID(guid, &cwAsioDriverCLSID)) {
            *ptr = nullptr;
            return E_NOINTERFACE;
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
        // ... (insert your code here)
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
        // ... (insert your code here)
        return ASE_OK;
    }

    cwASIOError outputReady() {
        // ... (insert your code here)
        return ASE_OK;
    }

private:
    static struct cwASIODriverVtbl const vtbl;

    std::atomic_ulong references;    // threadsafe reference counter
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
