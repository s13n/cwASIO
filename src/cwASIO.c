/** @file       cwASIO.c
 *  @brief      cwASIO native API
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */

#include "cwASIO.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#   define NOMINMAX
#   define WIN32_LEAN_AND_MEAN 
#   include <Windows.h>
#   include <combaseapi.h>
#   include <guiddef.h>
#   include <unknwnbase.h>
#else
#   include <alloca.h>
#   include <dirent.h>
#   include <dlfcn.h>
#   include <errno.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <sys/stat.h>
#endif

#ifdef _WIN32

static char *toUTF8(wchar_t const *wstr) {
    if (wstr) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (len > 0) {
            char *buf = malloc(len);
            len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf, len, NULL, NULL);
            if (len > 0)
                return buf;
            free(buf);
        }
    }
    return NULL;
};

static wchar_t *fromUTF8(char const *str) {
    if (str) {
        int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        if (len > 0) {
            wchar_t *buf = malloc(len * sizeof(wchar_t));
            len = MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, len);
            if (len > 0)
                return buf;
            free(buf);
        }
    }
    return NULL;
};

long cwASIOload(char const *key, struct cwASIODriver **drv) {
    CLSID id;
    HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(res))
        return res;
    cwASIOtoGUID(key, &id);
    // ASIO (ab)uses the CLSID for the IID, so we use the same ID twice here
    res = CoCreateInstance(&id, NULL, CLSCTX_INPROC_SERVER, &id, drv);
    if (FAILED(res)) {
        CoUninitialize();
        return res;
    }
    return 0;
}

void cwASIOunload(struct cwASIODriver *drv) {
    if(drv)
        drv->lpVtbl->release(drv);
    CoUninitialize();
}

static LSTATUS getValue(HKEY hkey, wchar_t *subKey, wchar_t *name, wchar_t **val, DWORD *len) {
    LSTATUS err = ERROR_SUCCESS;
    for (;;) {    // try until buffer size is sufficient
        err = RegGetValueW(hkey, subKey, name, RRF_RT_REG_SZ, NULL, *val, len);
        if (err != ERROR_MORE_DATA)
            break;
        wchar_t *tmp = *val;
        *val = realloc(tmp, *len * sizeof(wchar_t));
        if (!*val)
            free(tmp);
    }
    if (*val && err != ERROR_SUCCESS)
        (*val)[0] = '\0';
    return err;
}

int cwASIOenumerate(cwASIOcallback *cb, void *context) {
    HKEY hkey;
    LSTATUS err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASIO", 0, KEY_READ, &hkey);
    if (err != ERROR_SUCCESS)
        return err;

    DWORD subkeyLen;
    err = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, &subkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (err != ERROR_SUCCESS)
        goto close_hkey;

    wchar_t *subKey = malloc(sizeof(wchar_t) * (subkeyLen + 1));
    DWORD clsidLen = 40;
    wchar_t *clsid = malloc(sizeof(wchar_t) * clsidLen);
    DWORD descriptionLen = 16;
    wchar_t *description = malloc(sizeof(wchar_t) * descriptionLen);
    for (DWORD index = 0; err != ERROR_NO_MORE_ITEMS; ++index) {
        if (!subKey || !clsid || !description)
            break;

        DWORD nameLen = subkeyLen + 1;      // account for terminating NUL
        err = RegEnumKeyExW(hkey, index, subKey, &nameLen, NULL, NULL, NULL, NULL);
        if (err != ERROR_SUCCESS)
            continue;

        err = getValue(hkey, subKey, L"CLSID", &clsid, &clsidLen);
        err = getValue(hkey, subKey, L"Description", &description, &descriptionLen);

        char *nm = toUTF8(subKey);
        char *id = toUTF8(clsid);
        char *de = toUTF8(description);
        if (cb(context, nm, id, de))
            err = ERROR_SUCCESS;            // continue iteration if enumerate returns true
        else
            err = ERROR_NO_MORE_ITEMS;      // terminate iteration if enumerate returns false
        free(de);
        free(id);
        free(nm);
    }
    err = ERROR_SUCCESS;
    free(description);
    free(clsid);
    free(subKey);
close_hkey:
    RegCloseKey(hkey);
    return err;
}

int cwASIOgetParameter(char const *name, char const *key, char *buffer, unsigned size) {
    enum {subkeysize = 256, buffersize=2048};
    if(!name)
        return ASE_InvalidParameter;
    wchar_t subkey[subkeysize] = L"SOFTWARE\\ASIO\\";
    int n = wcslen(subkey);     // remember length so far for appending
    n = MultiByteToWideChar(CP_UTF8, 0, name, -1, subkey + n, subkeysize - n);      // append name
    if(n <= 0)
        return ASE_InvalidParameter;
    wchar_t *value = fromUTF8(key);
    if(!value) {    // we just check if the subkey exists
        HKEY hk;
        LSTATUS stat = RegOpenKeyW(HKEY_LOCAL_MACHINE, subkey, &hk);
        if(stat != ERROR_SUCCESS)
            return ASE_NotPresent;
        RegCloseKey(hk);
        return ASE_OK;
    }
    wchar_t buf[buffersize];
    DWORD bufsize = buffersize;
    LSTATUS err = RegGetValueW(HKEY_LOCAL_MACHINE, subkey, value, RRF_RT_REG_SZ, NULL, buf, &bufsize);
    if (err) {
        free(value);
        return ASE_NotPresent;
    }
    n = WideCharToMultiByte(CP_UTF8, 0, buf, bufsize, buffer, size, NULL, NULL);
    free(value);
    if(n <= 0)
        return ASE_NotPresent;
    return n <= (int)size ? n : (int)size;
}

#else

typedef struct cwASIODriver * (CWASIO_METHOD InstantiateDriver)(void);

long cwASIOload(char const *id, struct cwASIODriver **drv) {
    void *lib = dlopen(id, RTLD_LOCAL | RTLD_NOW);
    if(!lib)
        return ASE_NotPresent;

    InstantiateDriver *instantiateDriver = dlsym(lib, "instantiateDriver");
    if (!instantiateDriver)
        return ASE_NotPresent;

    *drv = instantiateDriver();

    return *drv ? ASE_OK : ASE_NotPresent;
}

void cwASIOunload(struct cwASIODriver *drv) {
    if(drv)
        drv->lpVtbl->release(drv);
}

static char *cwASIOreadConfig(char const *base, char const *name, char const *file) {
    size_t baseLen = strlen(base);
    size_t nameLen = strlen(name);
    char *path = (char *)alloca(baseLen + 1 + nameLen + 1 + strlen(file) + 1);
    strcpy(path, base);
    path[baseLen] = '/';
    strcpy(path + baseLen + 1, name);
    path[baseLen + 1 + nameLen] = '/';
    strcpy(path + baseLen + 1 + nameLen + 1, file);

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        int err = errno;
        close(fd);
        errno = err;
        return NULL;
    }

    size_t size = st.st_size;
    if (size > 1023)
        size = 1023;  // limit buffer size
    char *txt = (char *)malloc(size + 1);
    ssize_t len = read(fd, txt, size);
    int err = errno;
    close(fd);
    if (len < 0) {
        free(txt);
        errno = err;
        return NULL;
    }
    else {
        txt[len] = '\0';
        char *end = strchr(txt, '\n');
        if (end)
            *end = '\0';    // terminate at end of first line
        errno = err;
        return txt;
    }
}

int cwASIOgetParameter(char const *name, char const *key, char *buffer, unsigned size) {
    if(!key) {
        char path[2048];
        int n = snprintf(path, sizeof(path), "/etc/cwASIO/%s", name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
            return 0;
        else
            return ASE_NotPresent;
    }

    int ret = 0;
    char *val = cwASIOreadConfig("/etc/cwASIO", name, key);
    if (val && buffer && size > 0) {
        strncpy(buffer, val, size);
        ret = strlen(buffer);
    }
    free(val);
    return ret;
}

int cwASIOenumerate(cwASIOcallback *cb, void *context) {
    static char const *path = "/etc/cwASIO";
    int res = 0;
    struct dirent *rent;
    DIR *base = opendir(path);

    if (!base)
        return errno;

    do {
        errno = 0;
        rent = readdir(base);
        if (!rent) {
            res = errno;
            break;
        }
        else if (rent->d_name[0] == '.') {
            continue;   // ignore entries starting with a dot
        }
        else {
            char *driver = cwASIOreadConfig(path, rent->d_name, "driver");
            char *description = cwASIOreadConfig(path, rent->d_name, "description");
            if (!cb(context, rent->d_name, driver, description))
                rent = NULL;
            free(driver);
            free(description);
        }
    } while (rent);

    closedir(base);
    return res;
}

#endif

bool cwASIOcompareGUID(cwASIOGUID const *a, cwASIOGUID const *b) {
    return a && b ? 0 == memcmp(a, b, sizeof(cwASIOGUID)) : a == b;
}

bool cwASIOtoGUID(char const *clsid, cwASIOGUID *guid) {
    if (!clsid || !guid)
        return false;
    int n = sscanf(clsid, "{%8" SCNx32 "-%4" SCNx16 "-%4" SCNx16 "-%2" SCNx8 "%2" SCNx8 "-%2" SCNx8 "%2" SCNx8 "%2" SCNx8 "%2" SCNx8 "%2" SCNx8 "%2" SCNx8 "}"
        , &guid->Data1, &guid->Data2, &guid->Data3
        , &guid->Data4[0], &guid->Data4[1], &guid->Data4[2], &guid->Data4[3]
        , &guid->Data4[4], &guid->Data4[5], &guid->Data4[6], &guid->Data4[7]);
    return n == 11;
}

/** @}*/
