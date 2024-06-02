/** @file       cwASIO.cpp
 *  @brief      cwASIO API (Windows implementation)
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */

#include "cwASIOifc.hpp"
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

#if CWASIO_USE_WTL4MINGW
    #include "atlport/include/atlbase.h"
#else
    #include <atlbase.h>
#endif

struct AsioDriver {
    IASIO *theAsioDriver_;
    unsigned initCounter_;

    AsioDriver() : theAsioDriver_(nullptr), initCounter_(0) {}
    AsioDriver(IASIO *theAsioDriver) : theAsioDriver_(theAsioDriver), initCounter_(0) {}
    IASIO *get() {
        return theAsioDriver_;
    }
    IASIO *operator=(IASIO *theAsioDriver) {
        theAsioDriver_ = theAsioDriver;
        if (!theAsioDriver_)
            initCounter_ = 0;
        return theAsioDriver_;
    }
    void addRef() {
        ++initCounter_;
    }
    bool decRef() {
        if (initCounter_ == 0)
            return false;
        --initCounter_;
        return true;
    }
};

static AsioDriver theAsioDriver;


cwASIOError ASIOLoad(char const* path) {
    if (theAsioDriver.get())
        return ASE_NoMemory;
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE)
        return ASE_NoMemory;
    HRESULT hres;
    theAsioDriver = cwASIOload(path, hres);
    if (!theAsioDriver.get()) {
        ::CoUninitialize();
        return ASE_NotPresent;
    }
    return ASE_OK;
}

cwASIOError ASIOUnload(void) {
    if (!theAsioDriver.get())
        return ASE_InvalidParameter; // do not call CoUninitialize here, because it wasn't yet called
    cwASIOunload(theAsioDriver.get());
    theAsioDriver = nullptr;
    ::CoUninitialize();
    return ASE_OK;
}

cwASIOError ASIOInit(cwASIODriverInfo *info) {
    if(!theAsioDriver.get())
        return ASE_InvalidParameter;
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE)
        return ASE_NotPresent;
    info->asioVersion = 2;
    theAsioDriver.get()->getDriverName(info->name);
    info->driverVersion = theAsioDriver.get()->getDriverVersion();
    if (!theAsioDriver.get()->init(info->sysRef)) {
        theAsioDriver.get()->getErrorMessage(info->errorMessage);
        ::CoUninitialize();
        return ASE_HWMalfunction;
    }
    info->errorMessage[0] = '\0';
    theAsioDriver.addRef();
    return ASE_OK;
}

cwASIOError ASIOExit(void) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    if (theAsioDriver.decRef())
        ::CoUninitialize();
    return ASE_OK;
}

cwASIOError ASIOStart(void) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->start();
}

cwASIOError ASIOStop(void) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->stop();
}

cwASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getChannels(numInputChannels, numOutputChannels);
}

cwASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getLatencies(inputLatency, outputLatency);
}

cwASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getBufferSize(minSize, maxSize, preferredSize, granularity);
}

cwASIOError ASIOCanSampleRate(cwASIOSampleRate sampleRate) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->canSampleRate(sampleRate);
}

cwASIOError ASIOGetSampleRate(cwASIOSampleRate *currentRate) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getSampleRate(currentRate);
}

cwASIOError ASIOSetSampleRate(cwASIOSampleRate sampleRate) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->setSampleRate(sampleRate);
}

cwASIOError ASIOGetClockSources(cwASIOClockSource *clocks, long *numSources) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getClockSources(clocks, numSources);
}

cwASIOError ASIOSetClockSource(long reference) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->setClockSource(reference);
}

cwASIOError ASIOGetSamplePosition (cwASIOSamples *sPos, cwASIOTimeStamp *tStamp) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getSamplePosition(sPos, tStamp);
}

cwASIOError ASIOGetChannelInfo(cwASIOChannelInfo *info) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->getChannelInfo(info);
}

cwASIOError ASIOCreateBuffers(cwASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, cwASIOCallbacks const *callbacks) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->createBuffers(bufferInfos, numChannels, bufferSize, callbacks);
}

cwASIOError ASIODisposeBuffers(void) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->disposeBuffers();
}

cwASIOError ASIOControlPanel(void) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->controlPanel();
}

cwASIOError ASIOFuture(long selector, void *params) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->future(selector, params);
}

cwASIOError ASIOOutputReady(void) {
    if(!theAsioDriver.get())
        return ASE_NotPresent;
    return theAsioDriver.get()->outputReady();
}


int cwASIOenumerate(cwASIOcallback *cb, void *context) {
    HKEY key;
    LSTATUS err = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASIO", 0, KEY_READ, &key);
    if (err != ERROR_SUCCESS)
        return err;
    std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(&::RegCloseKey)> hkey(key, &::RegCloseKey);

    DWORD subkeyLen;
    err = ::RegQueryInfoKeyW(hkey.get(), nullptr, nullptr, nullptr, nullptr, &subkeyLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (err != ERROR_SUCCESS)
        return err;

    auto toUTF8 = [](std::wstring const& wstr) {
        std::string res;
        if (!wstr.empty()) {
            res.resize(::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr));
            if (!res.empty()) {
                if (int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, res.data(), int(res.size()), nullptr, nullptr))
                    res.resize(len - 1);
                else
                    res.clear();
            }
        }
        return res;
    };

    for (DWORD index = 0; err != ERROR_NO_MORE_ITEMS; ++index) {
        std::wstring name;
        name.resize(subkeyLen);
        auto nameLen = subkeyLen + 1;       // account for terminating NULL
        err = ::RegEnumKeyExW(hkey.get(), index, name.data(), &nameLen, nullptr, nullptr, nullptr, nullptr);
        if (err != ERROR_SUCCESS)
            continue;
        name.resize(nameLen);

        std::wstring clsid;
        auto clsidLen = DWORD(clsid.capacity());
        do {    // try until buffer size is sufficient
            clsid.resize(clsidLen++);       // account for terminating NULL
            err = ::RegGetValueW(hkey.get(), name.c_str(), L"CLSID", RRF_RT_REG_SZ, nullptr, clsid.data(), &clsidLen);
        } while (err == ERROR_MORE_DATA);
        if (err != ERROR_SUCCESS)
            clsid.clear();

        std::wstring description;
        auto descriptionLen = DWORD(description.capacity());
        do {    // try until buffer size is sufficient
            description.resize(descriptionLen++);       // account for terminating NULL
            err = ::RegGetValueW(hkey.get(), name.c_str(), L"Description", RRF_RT_REG_SZ, nullptr, description.data(), &descriptionLen);
        } while (err == ERROR_MORE_DATA);
        if (err != ERROR_SUCCESS)
            description.clear();

        if (cb(context, toUTF8(name).c_str(), toUTF8(clsid).c_str(), toUTF8(description).c_str()))
            err = ERROR_SUCCESS;            // continue iteration if enumerate returns true
        else
            err = ERROR_NO_MORE_ITEMS;      // terminate iteration if enumerate returns false
    }
    return ERROR_SUCCESS;
}

IASIO *cwASIOload(char const *key, HRESULT &res) {
    CLSID id;
    USES_CONVERSION;
    if (auto clsid = A2COLE(key))
        res = CLSIDFromString(clsid, &id);
    else
        res = E_POINTER;
    if (FAILED(res))
        return nullptr;
    CComPtr<IClassFactory> factory;
    res = ::CoGetClassObject(id, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void**)&factory);
    if (FAILED(res))
        return nullptr;
    void* instance;
    res = factory->CreateInstance(NULL, id, &instance);
    if (FAILED(res) || !instance)
        return nullptr;
    return static_cast<IASIO*>(instance);
}

void cwASIOunload(IASIO *ifc) {
    if (ifc)
        ifc->Release();
}

/** @}*/
