/** @file       cwASIOdriver.c
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * \addtogroup cwASIO
 *  @{
 */

#include "cwASIOdriver.h"
#include <assert.h>
#include <stdbool.h>

#ifdef _WIN32
#   include <olectl.h>
#   include <unknwnbase.h>
#   include <wchar.h>

#   define MODULE_EXPORT    // this is taken care of by the .def file
#else
#   define __USE_GNU
#   include <dlfcn.h>
#   include <errno.h>
#   include <fcntl.h>
#   include <stdio.h>
#   include <string.h>
#   include <time.h>
#   include <unistd.h>
#   include <sys/stat.h>

#   define MODULE_EXPORT __attribute__((visibility("protected")))
#endif

/** This file provides the scaffolding for implementing a cwASIO driver that can be loaded by a host application.
* The scaffolding arranges for object instantiation, discovery and installation. The driver functionality must
* be implemented elsewhere (see the provided skeleton files).
* 
* On Windows, this scaffolding includes a COM compliant class factory, and a few functions that are exported
* from the DLL, as defined in `cwASIOdriver.def`.
* 
* On Linux TBD
*/

struct IUnknown;
struct ClassFactory;

struct ClassFactoryVtbl {
    long (CWASIO_METHOD *QueryInterface)(struct ClassFactory *, cwASIOGUID const *, void **);
    unsigned long (CWASIO_METHOD *AddRef)(struct ClassFactory *);
    unsigned long (CWASIO_METHOD *Release)(struct ClassFactory *);
    long (CWASIO_METHOD *CreateInstance)(struct ClassFactory *, struct IUnknown *, cwASIOGUID const *, void **);
    long (CWASIO_METHOD *LockServer)(struct ClassFactory *, int);
};

struct ClassFactory {
    struct ClassFactoryVtbl const *lpVtbl;
};

static cwASIOGUID const iidIUnknown      = {0x00000000,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46};
static cwASIOGUID const iidIClassFactory = {0x00000001,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46};

#ifdef _WIN32
    typedef LONG dll_use_count_t;
    static LONG dllUseCount = 0;
    static dll_use_count_t updateDllUseCount(bool increaseNotDecrease) {
        if (increaseNotDecrease)
            return InterlockedIncrease(&dllUseCount);
        else
            return InterlockedDecrease(&dllUseCount);
    }
#elif __GNUC__
    typedef int dll_use_count_t;
    static int dllUseCount = 0;
    static int updateDllUseCount(bool increaseNotDecrease) {
        if (increaseNotDecrease)
            return __sync_add_and_fetch(&dllUseCount, 1);
        else
            return __sync_sub_and_fetch(&dllUseCount, 1);
    }
#else
    #error unsupported compiler
#endif

static long CWASIO_METHOD queryInterface(struct ClassFactory *self, cwASIOGUID const *guid, void **ppv) {
    // Check if the GUID matches an IClassFactory or IUnknown IID.
    if (!cwASIOcompareGUID(guid, &iidIUnknown) && !cwASIOcompareGUID(guid, &iidIClassFactory)) {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    // It's a match! We fill in his handle with the same object pointer he passed us, i.e. the factory.
    *ppv = self;
    self->lpVtbl->AddRef(self);
    return 0;    // Let caller know he indeed has a factory.
}

static unsigned long CWASIO_METHOD addRef(struct ClassFactory *f) {
    dll_use_count_t result = updateDllUseCount(true);
    assert(result > 0);
    return result;
}

static unsigned long CWASIO_METHOD release(struct ClassFactory *f) {
    dll_use_count_t result = updateDllUseCount(false);
    assert(result >= 0);
    return result;
}

static long CWASIO_METHOD createInstance(struct ClassFactory *f, struct IUnknown *outer, cwASIOGUID const *guid, void **ppv) {
    long hr = 0;
    struct cwASIODriver *obj = NULL;
    *ppv = 0;   // Assume an error by clearing caller's handle.

    if (outer)
        return CLASS_E_NOAGGREGATION;   // We don't support aggregation.

    // Create our instance.
    obj = makeAsioDriver();
    if (!obj)
        return E_OUTOFMEMORY;

    // Let cwAsioDriver's QueryInterface check the GUID and set the pointer.
    // It also increments the reference count (to 2) if all goes well.
    hr = obj->lpVtbl->QueryInterface(obj, guid, ppv);

    // NOTE: If there was an error in QueryInterface(), then Release() will be decrementing
    // the count back to 0 and will delete the instance for us. One error that may occur is
    // that the caller is asking for some sort of object that we don't support (i.e. it's a
    // GUID we don't recognize).
    obj->lpVtbl->Release(obj);

    if (!hr)
        updateDllUseCount(true);

    return hr;
}

static long CWASIO_METHOD lockServer(struct ClassFactory *f, int flock) {
    if (flock)
        updateDllUseCount(true);
    else
        updateDllUseCount(false);

    return 0L;
}

static struct ClassFactoryVtbl driverFactoryVtbl = {
    .QueryInterface = &queryInterface,
    .AddRef = &addRef,
    .Release = &release,
    .CreateInstance = &createInstance,
    .LockServer = &lockServer
};

static struct ClassFactory driverFactory = { &driverFactoryVtbl };

#ifdef _WIN32

MODULE_EXPORT HRESULT CWASIO_METHOD DllGetClassObject(cwASIOGUID const *objGuid, cwASIOGUID const *factoryGuid, void **factoryHandle) {
    // Check that the caller is passing our GUID. That's the COM object our DLL implements.
    if (cwASIOcompareGUID(objGuid, &cwAsioDriverCLSID)) {
        // Fill in the caller's handle with a pointer to our factory object. We'll let our queryInterface do that, because it also
        // checks the IClassFactory GUID and does other book-keeping.
        return queryInterface(&driverFactory, factoryGuid, factoryHandle);
    } else {
        // We don't understand this GUID. It's obviously not for our DLL.
        // Let the caller know this by clearing his handle and returning CLASS_E_CLASSNOTAVAILABLE.
        *factoryHandle = NULL;
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}

MODULE_EXPORT HRESULT CWASIO_METHOD DllCanUnloadNow() {
    // If someone has retrieved pointers to any of our objects, and not yet Release()'ed them,
    // then we return false to indicate not to unload this DLL.
    // Also, if someone has us locked, return false
    assert(dllUseCount >= 0);
    return dllUseCount <= 0 ? S_OK : S_FALSE;
}

static HMODULE ownModule = NULL;

static void stringFromGUID(cwASIOGUID const *guid, wchar_t *buffer) {
    swprintf(buffer, 37, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
        , guid->Data1, guid->Data2, guid->Data3
        , guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3]
        , guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static DWORD sizeInChars(wchar_t const *string) {
    return (DWORD)(sizeof(wchar_t) * (wcslen(string)+1));
}

/** Put registration info into registry.
 * This function is called by installers, or by `regsvr32.exe`, to create the registry entries
 * required to enumerate the driver on Windows systems. It determines the path to the driver
 * from the running module, so the installer should put the driver DLL into its final place
 * before loading the DLL and calling this function.
 */
MODULE_EXPORT HRESULT CWASIO_METHOD DllRegisterServer(void) {
    enum {buffersize = 2048};   // the maximum string size MS recommends in the registry for performance reasons
    LSTATUS err = 0;
    //write the default value
    wchar_t buffer[buffersize];
    int n = MultiByteToWideChar(CP_UTF8, 0, cwAsioDriverDescription, -1, buffer, buffersize);
    if(n <= 0)
        return HRESULT_FROM_WIN32(GetLastError());
    wchar_t subkey[256] = L"CLSID\\";
    stringFromGUID(&cwAsioDriverCLSID, subkey + wcslen(subkey));    // append CLSID
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, NULL, REG_SZ, buffer, (DWORD)(sizeof(wchar_t) * n));
    if (err)
        return HRESULT_FROM_WIN32(err);
    n = wcslen(subkey);     // remember length so far for further appending
    //write the HKCR\CLSID\{---}\InprocServer32 default key, i.e. the path to the DLL
    GetModuleFileNameW(ownModule, buffer, buffersize);
    wcscpy(subkey + n, L"\\InprocServer32");
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, NULL, REG_SZ, buffer, sizeInChars(buffer));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the HKCR\CLSID\{---}\InprocServer32\\ThreadingModel value
    wchar_t thmod[] = L"Both";
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, L"ThreadingModel", REG_SZ, thmod, sizeInChars(thmod));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the "ProgId" key data under HKCR\CLSID\{---}\ProgID
    wcscpy(subkey + n, L"\\ProgID");
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, NULL, REG_SZ, cwAsioDriverProgID, sizeInChars(cwAsioDriverProgID));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the "CLSID" entry data under HKLM\SOFTWARE\ASIO\<key>
    stringFromGUID(&cwAsioDriverLibID, buffer);
    wcscpy(subkey, L"SOFTWARE\\ASIO\\");
    n = wcslen(subkey);     // remember length so far for appending
    n += MultiByteToWideChar(CP_UTF8, 0, cwAsioDriverKey, -1, buffer + n, buffersize - n);      // append Key
    err = RegSetKeyValueW(HKEY_LOCAL_MACHINE, subkey, L"CLSID", REG_SZ, buffer, (DWORD)(sizeof(wchar_t) * n));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the "Description" entry data under HKLM\SOFTWARE\ASIO\<key>
    n = MultiByteToWideChar(CP_UTF8, 0, cwAsioDriverDescription, -1, buffer, buffersize);
    err = RegSetKeyValueW(HKEY_LOCAL_MACHINE, subkey, L"Description", REG_SZ, buffer, sizeInChars(buffer));
    return HRESULT_FROM_WIN32(err);
}

/** Remove registration info from registry.
 * This function removes what `DllRegisterServer` has added.
 */
MODULE_EXPORT HRESULT CWASIO_METHOD DllUnregisterServer(void) {
    LSTATUS err = 0;
    //remove the entire tree in HKLM\SOFTWARE\ASIO
    wchar_t subkey[256] = L"SOFTWARE\\ASIO\\";
    int n = wcslen(subkey);     // remember length so far for appending
    MultiByteToWideChar(CP_UTF8, 0, cwAsioDriverKey, -1, subkey + n, 256 - n);      // append Key
    err = RegDeleteTreeW(HKEY_LOCAL_MACHINE, subkey);
    if (err)
        return HRESULT_FROM_WIN32(err);
    //remove the entire tree in HKCR\clsid
    wcscpy(subkey, L"CLSID\\");
    stringFromGUID(&cwAsioDriverCLSID, subkey + wcslen(subkey));    // append CLSID
    err = RegDeleteTreeW(HKEY_CLASSES_ROOT, subkey);
    return HRESULT_FROM_WIN32(err);
}

MODULE_EXPORT BOOL CWASIO_METHOD DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:        // Initialize once for each new process.
        if (reserved)
            return FALSE;           // We don't support static load!
        else
            ownModule = hinst;
        break;

    case DLL_THREAD_ATTACH:         // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:         // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:
        ownModule = NULL;
        if (reserved != NULL) {
            break; // do not do cleanup if process termination scenario
        }

        // Perform any necessary cleanup.
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#else // not _WIN32

/** Put registration info into /etc/cwASIO.
 *
 * This function is called by installers to register the driver with the system.
 * It is required for enumerating the driver on a Linux system. It determines
 * the path to the driver from the running module, so the installer should put
 * the driver shared object into its final place before loading it and calling
 * this function.
 * 
 * Note that the `/etc/cwASIO` directory must exist and be writable, please
 * ensure that before calling this function, otherwise this function fails.
 */
MODULE_EXPORT long CWASIO_METHOD registerDriver(void) {
    char buf[2048];
    //assemble the path
    int n = snprintf(buf, sizeof(buf), "/etc/cwASIO/%s", cwAsioDriverKey);
    if(n < 0 || n >= sizeof(buf)-20)    // leave a reserve for later appending
        return EINVAL;
    //make the driver's registration directory
    if(0 != mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        return errno;
    //write the "driver" file under /etc/cwASIO/<key>
    Dl_info info;
    if(!dladdr(&registerDriver, &info))
        return EINVAL;
    strcpy(buf+n, "/driver");
    int fd = creat(buf, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if(fd < 0)
        return errno;
    int ret = write(fd, info.dli_fname, strlen(info.dli_fname));
    int err = errno;
    close(fd);
    if(ret < 0)
        return err;
    //write the "description" file under /etc/cwASIO/<key>
    strcpy(buf+n, "/description");
    fd = creat(buf, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if(fd < 0)
        return errno;
    ret = write(fd, cwAsioDriverDescription, strlen(cwAsioDriverDescription));
    err = errno;
    close(fd);
    if(ret < 0)
        return err;
    return 0;
}

/** Remove registration info. This function removes what `registerDriver` has
 * added.
 *
 * Note that only the two files `driver` and `description` are removed. If the
 * directory isn't empty thereafter, it won't be removed, in order to preserve
 * any data that was added in a different way. If the caller wants to ensure
 * that the directory gets deleted, too, it needs to remove all other files
 * before calling this function.
 */
MODULE_EXPORT long CWASIO_METHOD unregisterDriver(void) {
    char buf[2048];
    //assemble the path
    int n = snprintf(buf, sizeof(buf), "/etc/cwASIO/%s", cwAsioDriverKey);
    if(n < 0 || n >= sizeof(buf)-20)    // leave a reserve for later appending
        return EINVAL;

    strcpy(buf+n, "/description");
    if( 0 != unlink(buf))
        return errno;
    strcpy(buf+n, "/driver");
    if( 0 != unlink(buf))
        return errno;
    buf[n] = '\0';
    if(0 != rmdir(buf))
        return errno;
    return 0;
}

#endif

/** @}*/
