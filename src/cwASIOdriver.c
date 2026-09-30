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
#   include <dirent.h>
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
    if (objGuid) {
        // Fill in the caller's handle with a pointer to our factory object. We'll let our queryInterface do that, because it also
        // checks the IClassFactory GUID and does other book-keeping.
        return queryInterface(&driverFactory, factoryGuid, factoryHandle);
    } else {
        // No GUID provided. Let the caller know this by clearing his handle and returning CLASS_E_CLASSNOTAVAILABLE.
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

static bool equalIgnoringCase(wchar_t const *a, wchar_t const *b) {
    return CSTR_EQUAL == CompareStringOrdinal(a, -1, b, -1, TRUE);
}

// append a UTF-8 string to a wide string buffer of the given size (in characters)
static bool appendUTF8(wchar_t *buffer, size_t size, char const *str) {
    size_t n = wcslen(buffer);
    return MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer + n, (int)(size - n)) > 0;
}

// read a string value from the registry into a buffer of buffersize characters
static LSTATUS readString(HKEY hkey, wchar_t const *subkey, wchar_t const *value, wchar_t *buffer) {
    DWORD size = sizeof(wchar_t) * buffersize;
    return RegGetValueW(hkey, subkey, value, RRF_RT_REG_SZ, NULL, buffer, &size);
}

// get the full path of this DLL into a buffer of buffersize characters
static LSTATUS getOwnModulePath(wchar_t *buffer) {
    HMODULE ownModule;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (wchar_t*)&getOwnModulePath, &ownModule))
        return GetLastError();
    DWORD res = GetModuleFileNameW(ownModule, buffer, buffersize);
    if(res == 0)
        return GetLastError();
    if(res == buffersize)
        return ERROR_INSUFFICIENT_BUFFER;
    return ERROR_SUCCESS;
}

// get the path of the DLL registered for the given CLSID into a buffer of buffersize characters
static LSTATUS getRegisteredModulePath(wchar_t const *clsid, wchar_t *buffer) {
    wchar_t subkey[subkeysize];
    if(swprintf(subkey, subkeysize, L"CLSID\\%ls\\InprocServer32", clsid) < 0)
        return ERROR_INVALID_PARAMETER;
    return readString(HKEY_CLASSES_ROOT, subkey, NULL, buffer);
}

struct UsageContext {
    cwASIOGUID guid;
    bool used;
};

static bool usageCallback(void *context, char const *name, char const *id, char const *description) {
    struct UsageContext *ctx = context;
    cwASIOGUID guid;
    if (!cwASIOtoGUID(id, &guid) || !cwASIOcompareGUID(&ctx->guid, &guid))
        return true;
    ctx->used = true;
    return false;       // terminate enumeration
}

// check if any entry in HKLM\SOFTWARE\ASIO refers to the given CLSID; errs on the side of true
static bool isCLSIDused(wchar_t const *clsid) {
    char buffer[subkeysize];
    struct UsageContext ctx = { .used = true };
    if (WideCharToMultiByte(CP_UTF8, 0, clsid, -1, buffer, subkeysize, NULL, NULL) <= 0 || !cwASIOtoGUID(buffer, &ctx.guid))
        return true;
    ctx.used = false;
    if (0 != cwASIOenumerate(&usageCallback, &ctx))
        return true;
    return ctx.used;
}

/** Put registration info into registry.
 * This function is called by installers, or by `regsvr32.exe`, to create the registry entries
 * required to enumerate the driver on Windows systems. It determines the path to the driver
 * from the running module, so the installer should put the driver DLL into its final place
 * before loading the DLL and calling this function.
 *
 * Both the name and the CLSID need to be given in environment variables CWASIO_INSTALL_NAME and
 * CWASIO_INSTALL_CLSID, respectively. They can be discarded after the call returns.
 *
 * Registering again with the same name and CLSID from the same DLL succeeds and rewrites the entries.
 * The function fails with HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) without changing anything if the
 * name is already registered with a different CLSID, or if the CLSID is already registered for a
 * DLL at a different path.
 */
MODULE_EXPORT HRESULT CWASIO_METHOD DllRegisterServer(void) {
    char const *name = getenv("CWASIO_INSTALL_NAME");
    char const *clsid = getenv("CWASIO_INSTALL_CLSID");
    if (!name || !clsid)
        return HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
    wchar_t asioKey[subkeysize] = L"SOFTWARE\\ASIO\\";
    wchar_t clsidKey[subkeysize] = L"CLSID\\";
    wchar_t *wname = asioKey + wcslen(asioKey);
    wchar_t *wclsid = clsidKey + wcslen(clsidKey);
    if (!appendUTF8(asioKey, subkeysize, name) || !appendUTF8(clsidKey, subkeysize, clsid))
        return HRESULT_FROM_WIN32(GetLastError());
    wchar_t inprocKey[subkeysize];
    if (swprintf(inprocKey, subkeysize, L"%ls\\InprocServer32", clsidKey) < 0)
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    wchar_t modulePath[buffersize];
    LSTATUS err = getOwnModulePath(modulePath);
    if (err)
        return HRESULT_FROM_WIN32(err);
    //refuse to take over a name registered for a different CLSID
    wchar_t buffer[buffersize];
    err = readString(HKEY_LOCAL_MACHINE, asioKey, L"CLSID", buffer);
    if (err == ERROR_SUCCESS && !equalIgnoringCase(buffer, wclsid))
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
        return HRESULT_FROM_WIN32(err);
    //refuse to take over a CLSID registered for a different DLL
    err = readString(HKEY_CLASSES_ROOT, inprocKey, NULL, buffer);
    if (err == ERROR_SUCCESS && !equalIgnoringCase(buffer, modulePath))
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
        return HRESULT_FROM_WIN32(err);
    //write the HKCR\CLSID\{---} default value
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, clsidKey, NULL, REG_SZ, wname, sizeInChars(wname));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the HKCR\CLSID\{---}\InprocServer32 default value, i.e. the path to the DLL
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, inprocKey, NULL, REG_SZ, modulePath, sizeInChars(modulePath));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the HKCR\CLSID\{---}\InprocServer32\\ThreadingModel value
    wchar_t const *threadingModel = L"Both";
    err = RegSetKeyValueW(HKEY_CLASSES_ROOT, inprocKey, L"ThreadingModel", REG_SZ, threadingModel, sizeInChars(threadingModel));
    if (err)
        return HRESULT_FROM_WIN32(err);
    //write the "CLSID" entry data under HKLM\SOFTWARE\ASIO\<key>
    err = RegSetKeyValueW(HKEY_LOCAL_MACHINE, asioKey, L"CLSID", REG_SZ, wclsid, sizeInChars(wclsid));
    return HRESULT_FROM_WIN32(err);
}

/** Remove registration info from registry.
 * This function removes the entire registry entry under HKLM\SOFTWARE\ASIO, including values that
 * were added by others, e.g. the installer. It also removes the entry under HKCR\CLSID, unless it is
 * still used by another entry under HKLM\SOFTWARE\ASIO.
 *
 * The name needs to be given in the environment variable CWASIO_INSTALL_NAME. It can be discarded
 * after the call returns. The CLSID is found in the registry.
 *
 * The entry is only removed if it belongs to this driver, otherwise the function fails with
 * HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) without changing anything. The entry belongs to this driver
 * if the CLSID registered under the name is registered for this DLL. If the environment variable
 * CWASIO_INSTALL_CLSID is set, too, it must match the registered CLSID, and the DLL isn't checked.
 * This allows removing the entry from a DLL that has been moved since registration.
 */
MODULE_EXPORT HRESULT CWASIO_METHOD DllUnregisterServer(void) {
    char const *name = getenv("CWASIO_INSTALL_NAME");
    char const *clsid = getenv("CWASIO_INSTALL_CLSID");
    if (!name)
        return HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
    wchar_t asioKey[subkeysize] = L"SOFTWARE\\ASIO\\";
    if (!appendUTF8(asioKey, subkeysize, name))
        return HRESULT_FROM_WIN32(GetLastError());
    wchar_t registeredCLSID[buffersize];
    LSTATUS err = readString(HKEY_LOCAL_MACHINE, asioKey, L"CLSID", registeredCLSID);
    if (err)
        return HRESULT_FROM_WIN32(err);
    wchar_t clsidKey[subkeysize];
    if (swprintf(clsidKey, subkeysize, L"CLSID\\%ls", registeredCLSID) < 0)
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    //check that the entry belongs to this driver
    if (clsid) {
        wchar_t wclsid[subkeysize] = L"";
        if (!appendUTF8(wclsid, subkeysize, clsid))
            return HRESULT_FROM_WIN32(GetLastError());
        if (!equalIgnoringCase(wclsid, registeredCLSID))
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    } else {
        wchar_t modulePath[buffersize];
        wchar_t registeredPath[buffersize];
        err = getOwnModulePath(modulePath);
        if (err)
            return HRESULT_FROM_WIN32(err);
        err = getRegisteredModulePath(registeredCLSID, registeredPath);
        if (err || !equalIgnoringCase(modulePath, registeredPath))
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    //remove the entire tree in HKLM\SOFTWARE\ASIO
    err = RegDeleteTreeW(HKEY_LOCAL_MACHINE, asioKey);
    if (err)
        return HRESULT_FROM_WIN32(err);
    //remove the entire tree in HKCR\CLSID, unless another name still uses the CLSID
    if (!isCLSIDused(registeredCLSID)) {
        err = RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey);
        if (err == ERROR_FILE_NOT_FOUND)
            err = ERROR_SUCCESS;
    }
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
 * Registering again under the same name from the same shared object succeeds.
 * If the name is already registered for a different shared object, the function
 * fails with EEXIST without changing anything.
 *
 * The function returns 0 on success, otherwise it returns an errno value.
 */
MODULE_EXPORT int registerDriver(char const *name) {
    char buf[2048];
    Dl_info info;
    if(!dladdr(&registerDriver, &info))
        return EINVAL;
    //refuse to take over a name registered for a different driver
    if (cwASIOgetParameter(name, "driver", buf, sizeof(buf)) >= 0)
        return 0 == strcmp(buf, info.dli_fname) ? 0 : EEXIST;
    //assemble the path
    int n = snprintf(buf, sizeof(buf), "/etc/cwASIO/%s", name);
    if(n < 0 || n >= sizeof(buf)-20)    // leave a reserve for later appending
        return EINVAL;
    //make the driver's registration directory, unless it exists already
    if(0 != mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) && errno != EEXIST)
        return errno;
    //write the "driver" file under /etc/cwASIO/<key>
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

/* Remove the files in the directory whose path is in buf[0..n), except the file
 * `driver`. Subdirectories aren't removed, but make the function fail with
 * ENOTEMPTY. If dryRun is true, nothing is removed, but it is checked that
 * removal is possible. The contents of buf beyond n are modified.
 */
static int removeFiles(char *buf, size_t size, int n, bool dryRun) {
    DIR *dir = opendir(buf);
    if (!dir)
        return errno;
    int res = 0;
    struct dirent *ent;
    while (res == 0 && (errno = 0, ent = readdir(dir))) {
        if (0 == strcmp(ent->d_name, ".") || 0 == strcmp(ent->d_name, "..") || 0 == strcmp(ent->d_name, "driver"))
            continue;
        if ((size_t)snprintf(buf+n, size-n, "/%s", ent->d_name) >= size-n) {
            res = ENAMETOOLONG;
            break;
        }
        struct stat st;
        if (0 != lstat(buf, &st))
            res = errno == ENOENT ? 0 : errno;  // readdir() may still return entries we removed
        else if (S_ISDIR(st.st_mode))
            res = ENOTEMPTY;
        else if (!dryRun && 0 != unlink(buf))
            res = errno;
    }
    if (res == 0 && !ent)
        res = errno;    // readdir() failure, or 0 at the end of the directory
    closedir(dir);
    buf[n] = '\0';
    return res;
}

/** Remove registration info. This function removes what `registerDriver` has
 * added, i.e. the entire directory `/etc/cwASIO/<name>`, including any files
 * that were added in a different way, e.g. `description`.
 *
 * The entry is only removed if its `driver` file names this shared object,
 * otherwise the function fails with EACCES without changing anything. If the
 * directory contains subdirectories, the function fails with ENOTEMPTY without
 * changing anything. The caller needs to remove them before calling this
 * function.
 *
 * The function returns 0 on success, otherwise it returns an errno value.
 */
MODULE_EXPORT int unregisterDriver(char const *name) {
    char buf[2048];
    if (0 != cwASIOgetParameter(name, NULL, NULL, 0))
        return ENODEV;
    //check that the entry belongs to this driver
    Dl_info info;
    if(!dladdr(&registerDriver, &info))
        return EINVAL;
    if (cwASIOgetParameter(name, "driver", buf, sizeof(buf)) < 0 || 0 != strcmp(buf, info.dli_fname))
        return EACCES;
    //assemble the path
    int n = snprintf(buf, sizeof(buf), "/etc/cwASIO/%s", name);
    if(n < 0 || n >= sizeof(buf)-20)    // leave a reserve for later appending
        return EINVAL;
    //check for subdirectories before removing anything
    int err = removeFiles(buf, sizeof(buf), n, true);
    if (err == 0)
        err = removeFiles(buf, sizeof(buf), n, false);
    if (err)
        return err;
    //remove the "driver" file last, so the entry stays intact if anything fails before
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
        if (ctx->buf[ctx->len - 1] == '\0')
            ctx->len = strlen(ctx->buf);    // name fits, else len stays == size
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
