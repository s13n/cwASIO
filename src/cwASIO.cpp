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
#include "atlbase.h"
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>


static IASIO *theAsioDriver = nullptr;
static CLSID clsid{};


ASIOError ASIOLoad(char const* path) {
    if (theAsioDriver)
        return ASE_NoMemory;
    HRESULT hres;
    theAsioDriver = cwASIOload(path, hres);
    return theAsioDriver ? ASE_OK : ASE_NotPresent;
}

ASIOError ASIOInit(ASIODriverInfo *info) {
    if(theAsioDriver || !info)
        return ASE_InvalidParameter;

    CComPtr<IClassFactory> factory;
    HRESULT hr = ::CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void**)&factory);
    if(FAILED(hr)) {
        strcpy_s(info->errorMessage, sizeof(info->errorMessage), "getting class factory");
        return ASE_NotPresent;
    }
    void *instance;
    hr = factory->CreateInstance(NULL, clsid, &instance);
    if(FAILED(hr) || !instance) {
        strcpy_s(info->errorMessage, sizeof(info->errorMessage), "creating instance");
        return ASE_NotPresent;
    }
    theAsioDriver = static_cast<IASIO*>(instance);
    if(!theAsioDriver)
        return ASE_NotPresent;
    hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != S_FALSE)
        return ASE_NotPresent;
    theAsioDriver->AddRef();
    info->asioVersion = 2;
    theAsioDriver->getDriverName(info->name);
    info->driverVersion = theAsioDriver->getDriverVersion();
    theAsioDriver->getErrorMessage(info->errorMessage);
    return ASE_OK;
}

ASIOError ASIOExit(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    theAsioDriver->Release();
    ::CoUninitialize();
    theAsioDriver = 0;
    return ASE_OK;
}

ASIOError ASIOStart(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->start();
}

ASIOError ASIOStop(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->stop();
}

ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getChannels(numInputChannels, numOutputChannels);
}

ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getLatencies(inputLatency, outputLatency);
}

ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getBufferSize(minSize, maxSize, preferredSize, granularity);
}

ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->canSampleRate(sampleRate);
}

ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getSampleRate(currentRate);
}

ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->setSampleRate(sampleRate);
}

ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getClockSources(clocks, numSources);
}

ASIOError ASIOSetClockSource(long reference) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->setClockSource(reference);
}

ASIOError ASIOGetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getSamplePosition(sPos, tStamp);
}

ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->getChannelInfo(info);
}

ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->createBuffers(bufferInfos, numChannels, bufferSize, callbacks);
}

ASIOError ASIODisposeBuffers(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->disposeBuffers();
}

ASIOError ASIOControlPanel(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->controlPanel();
}

ASIOError ASIOFuture(long selector, void *params) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->future(selector, params);
}

ASIOError ASIOOutputReady(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return theAsioDriver->outputReady();
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
    static_cast<IASIO*>(instance)->AddRef();
    return static_cast<IASIO*>(instance);
}

void cwASIOunload(IASIO *ifc) {
    if (ifc)
        ifc->Release();
}

/** @}*/
