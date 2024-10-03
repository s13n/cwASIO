/** @file       cwASIOdriver.h
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * \addtogroup cwASIO
 *  @{
 */
#pragma once

#include "cwASIO.h"
#include <stdatomic.h>

#ifdef _WIN32
#   include <guiddef.h>
#   include <Winerror.h>
#else

// we try to mimick the Windows-specific error types to Linux errors.
enum {
    E_OUTOFMEMORY = ENOMEM,
    E_NOINTERFACE = EBADR,
    E_FAIL = ENXIO,
    CLASS_E_CLASSNOTAVAILABLE = ENOPROTOOPT,
    CLASS_E_NOAGGREGATION = EOPNOTSUPP
};

struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
};

#endif

/** The COM class GUID of the driver.
 * This is a GUID that identifies the driver, and thus is driver specific. The driver implementer
 * must provide the definition.
 */
extern cwASIOGUID const cwAsioDriverCLSID;

/** The Library GUID of the driver.
 * This is a GUID that identifies the driver DLL, and thus is driver specific. The driver implementer
 * must provide the definition.
 */
extern cwASIOGUID const cwAsioDriverLibID;

/** The key string (UTF-8) for the driver.
 * This will be used in driver registration. Keep its length reasonable.
 */
extern char const *cwAsioDriverKey;

/** The description string (UTF-8) for the driver.
 * This will be used in driver registration. Keep its length reasonable.
 */
extern char const *cwAsioDriverDescription;

/** The ProgID string to use under Windows.
 * This will be used in driver registration under Windows.
 */
extern wchar_t const *cwAsioDriverProgID;

/** Counter of active instances managed by this module.
 * As long as the count is greater than zero, the module may not be unloaded.
 * We use a threadsafe counter to support arbitrary threading arrangements.
 */
extern atomic_uint activeInstances;

/** Make an instance of the driver.
 * Implement this function to create an instance of the driver object and return a pointer to it.
 * 
 * Most of the initialization will happen when the `init()` method of the driver instance is called,
 * so what you need to do here is to allocate the driver object, and initialize the pointer to its
 * virtual function table (`cwASIODriverVtbl`). The virtual function table itself must also be
 * initialized with pointers to all the methods declared in `cwASIODriverVtbl`, i.e. you must
 * implement those methods, too. For details, see below.
 * 
 * @return Pointer to the new driver instance, or NULL on error.
 */
extern struct cwASIODriver *makeAsioDriver();

/** @}*/
