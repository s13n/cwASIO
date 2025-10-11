/** @file       cwASIOdriver.h
 *  @brief      cwASIO driver support
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
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

/** Entry describing a registration name and its corresponding GUID.
 * A driver will contain a table of those, terminated by an entry with a NULL ptr as the name.
 * Drivers with multiinstance capability will have more than one entry in this table. The first
 * entry is the "default" entry that is used unless a different name is given.
 */
struct cwASIOinstance {
    char const *name;   //!< The name under which the instance gets registered.
    cwASIOGUID guid;    //!< The GUID that corresponds to this name on Windows.
};

/** The list of instance names and GUIDs the driver supports.
 * This points to a fixed table in the driver that identifies the driver's supported instances.
 * Its content is driver specific. The driver implementer must provide the content. The last entry
 * must contain a NULL pointer in its name field.
 */
extern struct cwASIOinstance const cwAsioDriverInstances[];

/** Find the driver name corresponding to a GUID.
 * @param guid The GUID of the driver to find. Can be NULL.
 * @param buf The buffer that should be filled with the name. Must be non-null
 * when size>0.
 * @param size The size of the buffer in bytes. Should be at least 32.
 * @return The length of the name copied into buffer, or a negative error
 * number.
 *
 * This function only makes sense on Windows, where GUIDs are used to find the
 * driver. Under Linux the function would typically be called with a NULL
 * pointer for the GUID, and return zero, i.e. the function does nothing.
 *
 * Under Windows, the function scans through the ASIO registry to find the key
 * corresponding to the given GUID. When it finds it, it copies the name to the
 * given buffer and returns the name length. If the returned length is equal to
 * the given size, the buffer was too small. The function uses strncpy to fill
 * the buffer, so if the buffer size isn't sufficient, it won't be null
 * terminated. A good way to ensure null termination is to allocate a buffer
 * with one more byte, and fill that with a nullbyte.
 *
 * Negative return values are system error codes with the sign inverted. A zero
 * return value indicates that the GUID was found, but no name was copied. This
 * can be used for checking the presence of the GUID when you're not interested
 * in the name.
 */
long cwASIOfindName(cwASIOGUID const *guid, char *buf, size_t size);

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
