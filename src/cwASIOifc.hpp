/** @file       cwASIOifc.hpp
 *  @brief      cwASIO driver API (Windows)
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */
#pragma once

extern "C" {
    #include "cwASIO.h"
}
#include "unknwn.h"


__interface IASIO : public IUnknown {
    virtual long init(void *sysHandle) = 0;
    virtual void getDriverName(char *name) = 0;
    virtual long getDriverVersion() = 0;
    virtual void getErrorMessage(char *string) = 0;
    virtual long start() = 0;
    virtual long stop() = 0;
    virtual long getChannels(long *numInputChannels, long *numOutputChannels) = 0;
    virtual long getLatencies(long *inputLatency, long *outputLatency) = 0;
    virtual long getBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) = 0;
    virtual long canSampleRate(double sampleRate) = 0;
    virtual long getSampleRate(double *sampleRate) = 0;
    virtual long setSampleRate(double sampleRate) = 0;
    virtual long getClockSources(ASIOClockSource *clocks, long *numSources) = 0;
    virtual long setClockSource(long reference) = 0;
    virtual long getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp) = 0;
    virtual long getChannelInfo(ASIOChannelInfo *info) = 0;
    virtual long createBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks) = 0;
    virtual long disposeBuffers() = 0;
    virtual long controlPanel() = 0;
    virtual long future(long selector, void *opt) = 0;
    virtual long outputReady() = 0;
};


typedef bool (cwASIOcallback)(void*, char const*, char const*, char const*);

/** Enumerate devices from Windows registry.
* An enumeration function is passed to cwASIOenumerate(), which will get called once for every entry found in the ASIO driver list.
* If the enumeration function returns with a true result, enumeration continues with the next entry, otherwise enumeration terminates.
* Three strings are passed to the enumeration function: Name, CLSID and Description. The latter two might be empty, if the
* corresponding registry entry is empty or absent. You might want to ignore such entries.
* @param cb Pointer to callback function
* @param context Pointer to be forwarded to callback function as its first parameter.
* @return error code, which is zero on success.
*/
int cwASIOenumerate(cwASIOcallback *cb, void *context);

/** Load the ASIO driver.
* @param key On Windows, the CLSID of the driver to load.
* @param res A variable to receive an error code when unsuccessful.
* @return Pointer to the driver interface, or nullptr when unsuccessful.
*/
IASIO* cwASIOload(char const *key, HRESULT &res);

/** Unload the ASIO driver.
* @param ifc Pointer to the driver interface that was returned by cwASIOload()
*/
void cwASIOunload(IASIO *ifc);
