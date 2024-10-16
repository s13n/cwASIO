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

#ifdef _WIN32
#   include <guiddef.h>
#   include <Winerror.h>
#else
#   include <errno.h>
#   include <wchar.h>

// we try to mimick the Windows-specific error types to Linux errors.
enum {
    E_OUTOFMEMORY = ENOMEM,
    E_NOINTERFACE = EBADR,
    E_FAIL = ENXIO,
    CLASS_E_CLASSNOTAVAILABLE = ENOPROTOOPT,
    CLASS_E_NOAGGREGATION = EOPNOTSUPP
};
#endif

/** The COM class GUID of the driver.
 * This is a GUID that identifies the driver, and thus is driver specific. The driver implementer
 * must provide the definition.
 */
extern cwASIOGUID const cwAsioDriverCLSID;

/** The key string (UTF-8) for the driver.
 * This will be used in driver registration. Keep its length reasonable.
 */
extern char const *cwAsioDriverKey;

/** The description string (UTF-8) for the driver.
 * This will be used in driver registration. Keep its length reasonable.
 */
extern char const *cwAsioDriverDescription;

/** Make an instance of the driver.
 * This function must be implemented by the driver to create an instance of the driver object and
 * return a pointer to it.
 * 
 * Most of the initialization will happen when the `init()` method of the driver instance is called,
 * so what you need to do here is to allocate the driver object, and initialize the pointer to its
 * virtual function table (`cwASIODriverVtbl`). The virtual function table itself must also be
 * initialized with pointers to all the methods declared in `cwASIODriverVtbl`, i.e. you must
 * implement those methods, too. For details, see the provided skeleton files.
 * 
 * @return Pointer to the new driver instance, or NULL on error.
 */
extern struct cwASIODriver *makeAsioDriver();

/** @}*/
