/** @file       cwASIOdriver.c
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 * \addtogroup cwASIO
 *  @{
 */

#include "cwASIOdriver.h"
#include <assert.h>
#include <stdbool.h>

#ifdef _WIN32
#   include <windows.h>
#   include <olectl.h>
#   include <unknwnbase.h>
#   include <wchar.h>

#   define MODULE_EXPORT    // this is taken care of by the .def file
#else
#   define __USE_GNU
#   include <dlfcn.h>
#   include <errno.h>
#   include <fcntl.h>
#   include <link.h>
#   include <stdio.h>
#   include <string.h>
#   include <time.h>
#   include <unistd.h>
#   include <sys/stat.h>

#   define MODULE_EXPORT __attribute__((retain,visibility("default")))
#endif

/** This file provides the scaffolding for implementing a cwASIO driver that can be loaded by a host application.
* The scaffolding arranges for object instantiation, discovery and installation. The driver functionality must
* be implemented elsewhere (see the provided skeleton files).
*/

// Find the name in our table of instances.
static struct cwASIOinstance const *findName(char const *name) {
    if (!name)
        return cwAsioDriverInstances;       // first entry
    for (struct cwASIOinstance const *entry = cwAsioDriverInstances; entry->name; ++entry) {
        if (0 == strcmp(name, entry->name))
            return entry;
    }
    return NULL;
}

// Find the GUID in our table of instances.
static struct cwASIOinstance const *findGUID(cwASIOGUID const *objGuid) {
    for (struct cwASIOinstance const *entry = cwAsioDriverInstances; entry->name; ++entry) {
        if (cwASIOcompareGUID(objGuid, &entry->guid))
            return entry;
    }
    return NULL;
}


#ifdef _WIN32
/* On Windows, this scaffolding includes a COM compliant class factory, and a few functions that are exported
 * from the DLL, as defined in `cwASIOdriver.def`.
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

typedef LONG dll_use_count_t;
static LONG dllUseCount = 0;
static dll_use_count_t updateDllUseCount(bool increaseNotDecrease) {
    if (increaseNotDecrease)
        return InterlockedIncrement(&dllUseCount);
    else
        return InterlockedDecrement(&dllUseCount);
}

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
    hr = obj->lpVtbl->queryInterface(obj, guid, ppv);

    // NOTE: If there was an error in QueryInterface(), then Release() will be decrementing
    // the count back to 0 and will delete the instance for us. One error that may occur is
    // that the caller is asking for some sort of object that we don't support (i.e. it's a
    // GUID we don't recognize).
    obj->lpVtbl->release(obj);

    if (!hr)
        updateDllUseCount(true);

    return hr;
}

static long CWASIO_METHOD lockServer(struct ClassFactory *f, int flock) {
    updateDllUseCount(flock != 0);
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

MODULE_EXPORT HRESULT CWASIO_METHOD DllGetClassObject(cwASIOGUID const *objGuid, cwASIOGUID const *factoryGuid, void **factoryHandle) {
    // Check that the caller is passing one of our GUIDs. That's the COM object our DLL implements.
    struct cwASIOinstance const *entry = findGUID(objGuid);
    if (entry) {
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

static void stringFromGUID(cwASIOGUID const *guid, wchar_t *buffer) {
    swprintf(buffer, 39, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"
        , guid->Data1, guid->Data2, guid->Data3
        , guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3]
        , guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static DWORD sizeInChars(wchar_t const *string) {
    return (DWORD)(sizeof(wchar_t) * (wcslen(string)+1));
}

enum {
    subkeysize = 256,   // should be enough for keys including the driver name
    buffersize = 2048   // the maximum string size MS recommends in the registry for performance reasons
};

/** Put registration info into registry.
 * This function is called by installers, or by `regsvr32.exe`, to create the registry entries
 * required to enumerate the driver on Windows systems. It determines the path to the driver
 * from the running module, so the installer should put the driver DLL into its final place
 * before loading the DLL and calling this function.
 */
MODULE_EXPORT HRESULT CWASIO_METHOD DllRegisterServer(void) {
    LSTATUS err = 0;
    struct cwASIOinstance const *entry = findName(getenv("CWASIO_INSTALL_NAME"));
    if (!entry)
        return HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
    //write the default value
    wchar_t buffer[buffersize];
    int n = MultiByteToWideChar(CP_UTF8, 0, entry->name, -1, buffer, buffersize);
    if(n <= 0)
        return HRESULT_FROM_WIN32(GetLastError());
    wchar_t subkey[subkeysize] = L"CLSID\\";
    stringFromGUID(&entry->guid, subkey + wcslen(subkey));    // append CLSID
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, NULL, REG_SZ, buffer, (DWORD)(sizeof(wchar_t) * n));
    if (err)
        return HRESULT_FROM_WIN32(err);
    n = wcslen(subkey);     // remember length so far for further appending
    //write the HKCR\CLSID\{---}\InprocServer32 default key, i.e. the path to the DLL
    HMODULE ownModule;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (wchar_t*)&DllRegisterServer, &ownModule))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD res = GetModuleFileNameW(ownModule, buffer, buffersize);
    if(res == 0 || res == buffersize)
        return HRESULT_FROM_WIN32(GetLastError());
    wcscpy(subkey + n, L"\\InprocServer32");
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, NULL, REG_SZ, buffer, sizeInChars(buffer));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the HKCR\CLSID\{---}\InprocServer32\\ThreadingModel value
    wcscpy(buffer, L"Both");
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, subkey, L"ThreadingModel", REG_SZ, buffer, sizeInChars(buffer));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the "CLSID" entry data under HKLM\SOFTWARE\ASIO\<key>
    stringFromGUID(&entry->guid, buffer);
    wcscpy(subkey, L"SOFTWARE\\ASIO\\");
    n = wcslen(subkey);     // remember length so far for appending
    n = MultiByteToWideChar(CP_UTF8, 0, entry->name, -1, subkey + n, subkeysize - n);      // append Key
    if(n <= 0)
        return HRESULT_FROM_WIN32(GetLastError());
    err = RegSetKeyValueW(HKEY_LOCAL_MACHINE, subkey, L"CLSID", REG_SZ, buffer, sizeInChars(buffer));
    return HRESULT_FROM_WIN32(err);
}

/** Remove registration info from registry.
 * This function removes what `DllRegisterServer` has added.
 */
MODULE_EXPORT HRESULT CWASIO_METHOD DllUnregisterServer(void) {
    LSTATUS err = 0;
    struct cwASIOinstance const *entry = findName(getenv("CWASIO_INSTALL_NAME"));
    if (!entry)
        return HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
    //remove the entire tree in HKLM\SOFTWARE\ASIO
    wchar_t subkey[subkeysize] = L"SOFTWARE\\ASIO\\";
    int n = wcslen(subkey);     // remember length so far for appending
    MultiByteToWideChar(CP_UTF8, 0, entry->name, -1, subkey + n, subkeysize - n);      // append Key
    err = RegDeleteTreeW(HKEY_LOCAL_MACHINE, subkey);
    if (err)
        return HRESULT_FROM_WIN32(err);
    //remove the entire tree in HKCR\clsid
    wcscpy(subkey, L"CLSID\\");
    stringFromGUID(&entry->guid, subkey + wcslen(subkey));    // append CLSID
    err = RegDeleteTreeW(HKEY_CLASSES_ROOT, subkey);
    return HRESULT_FROM_WIN32(err);
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
 *
 * The name parameter should contain the name under which the driver should be
 * registered, i.e. the name of the directory within `/etc/cwASIO`. It may be
 * NULL, in which case the first name will be chosen from the list of those
 * supported.
 *
 * The function returns 0 on success, otherwise it returns an errno value.
 */
MODULE_EXPORT int registerDriver(char const *name) {
    char buf[2048];
    struct cwASIOinstance const *entry = findName(name);
    if (!entry)
        return ENODEV;
    //assemble the path
    int n = snprintf(buf, sizeof(buf), "/etc/cwASIO/%s", entry->name);
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
    return 0;
}

/** Remove registration info. This function removes what `registerDriver` has
 * added.
 *
 * Note that only the file `driver` is removed. If the directory isn't empty
 * thereafter, it won't be removed, in order to preserve any data that was added
 * in a different way. If the caller wants to ensure that the directory gets
 * deleted, too, it needs to remove all other files before calling this
 * function.
 *
 * The function returns 0 on success, otherwise it returns an errno value.
 */
MODULE_EXPORT int unregisterDriver(char const *name) {
    char buf[2048];
    struct cwASIOinstance const *entry = findName(name);
    if (!entry)
        return ENODEV;
    //assemble the path
    int n = snprintf(buf, sizeof(buf), "/etc/cwASIO/%s", entry->name);
    if(n < 0 || n >= sizeof(buf)-20)    // leave a reserve for later appending
        return EINVAL;

    strcpy(buf+n, "/driver");
    if( 0 != unlink(buf))
        return errno;
    buf[n] = '\0';
    if(0 != rmdir(buf))
        return errno;
    return 0;
}

// we assume clang or gcc here, or any other compiler whose atomics builtins are compatible
typedef int dll_use_count_t;
static int libUseCount = 0;
static int updateUseCount(bool increaseNotDecrease) {
    if (increaseNotDecrease)
        return __sync_add_and_fetch(&libUseCount, 1);
    else
        return __sync_sub_and_fetch(&libUseCount, 1);
}

static void *getLibraryHandle(void) {    
    // We iterate through the link_map list of the process until we find the address of our own object.
    // On Linux, the address of the link_map is the handle returned by dlopen.
    Dl_info info;
    if (dladdr(&registerDriver, &info)) {
        struct link_map *l;
        dlinfo(dlopen(NULL, RTLD_LAZY), RTLD_DI_LINKMAP, &l);
        while (l) {
            if (l->l_addr == (intptr_t)info.dli_fbase)
                return l;
            l = l->l_next;
        }
    }
    return NULL;
}

MODULE_EXPORT struct cwASIODriver *instantiateDriver(void) {
    // Create our instance.
    struct cwASIODriver *obj = makeAsioDriver();
    if (!obj)
        return NULL;

    void *ifc;
    // Let cwAsioDriver's QueryInterface set the pointer.
    // It also increments the reference count (to 2) if all goes well.
    long hr = obj->lpVtbl->queryInterface(obj, NULL, &ifc);

    // NOTE: If there was an error in QueryInterface(), then Release() will be decrementing
    // the count back to 0 and will delete the instance for us. One error that may occur is
    // that the caller is asking for some sort of object that we don't support (i.e. it's a
    // GUID we don't recognize).
    obj->lpVtbl->release(obj);

    if (hr != 0)
        return NULL;
    
    updateUseCount(true);
    return obj;
}

MODULE_EXPORT void releaseDriver(struct cwASIODriver *drv) {
    if(0 == updateUseCount(false)) {
        void *handle = getLibraryHandle();
        dlclose(handle);
    }
}

#endif

struct Findcontext {
    char *buf;
    size_t len;
    cwASIOGUID guid;
};

static bool findCallback(void *context, char const *name, char const *id, char const *description) {
    struct Findcontext *ctx = context;
    if (!ctx || !name || !id)
        return true;
    cwASIOGUID guid;
    if (!cwASIOtoGUID(id, &guid))
        return true;
    if(!cwASIOcompareGUID(&ctx->guid, &guid))
        return true;
    if (ctx->len > 0) {
        strncpy(ctx->buf, name, ctx->len);
        if (ctx->buf[ctx->len - 1])
            ctx->len = strlen(ctx->buf);
    }
    ctx->buf = NULL;    // success flag
    return false;       // terminate enumeration
}

MODULE_EXPORT long cwASIOfindName(cwASIOGUID const *guid, char *buf, size_t size) {
    if (!guid || (!buf && size > 0))
        return 0;
    struct Findcontext ctx = { size ? buf : (char*)1, size, *guid };
    int res = cwASIOenumerate(&findCallback, &ctx);
    if (res != 0)
        return -res;
    return ctx.buf ? -1 : ctx.len;
}

/** @}*/
