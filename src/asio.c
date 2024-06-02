/** @file       asio.c
 *  @brief      ASIO compatibility functions for cwASIO
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */

#include "asio.h"
#include "cwASIO.h"
#include <stdlib.h>


static struct cwAsioDriver *theAsioDriver = NULL;

ASIOError ASIOLoad(char const *path) {
    if(theAsioDriver)
        return ASE_NoMemory;
    return cwASIOload(path, &theAsioDriver);
}

ASIOError ASIOUnload(void) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_InvalidParameter;
    cwASIOunload(theAsioDriver);
    return ASE_OK;
}

ASIOError ASIOInit(ASIODriverInfo *info) {
    if(!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    if (!theAsioDriver->vtbl->init(theAsioDriver, info ? info->sysRef : 0))
        return ASE_NotPresent;
    if(info) {
        info->asioVersion = 2;
        theAsioDriver->vtbl->getDriverName(theAsioDriver, info->name);
        info->driverVersion = theAsioDriver->vtbl->getDriverVersion(theAsioDriver);
        theAsioDriver->vtbl->getErrorMessage(theAsioDriver, info->errorMessage);
    }
    return ASE_OK;
}

ASIOError ASIOExit(void) {
    return ASE_OK;
}

ASIOError ASIOStart(void) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->start(theAsioDriver);
}

ASIOError ASIOStop(void) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->stop(theAsioDriver);
}

ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getChannels(theAsioDriver, numInputChannels, numOutputChannels);
}

ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getLatencies(theAsioDriver, inputLatency, outputLatency);
}

ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getBufferSize(theAsioDriver, minSize, maxSize, preferredSize, granularity);
}

ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->canSampleRate(theAsioDriver, sampleRate);
}

ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getSampleRate(theAsioDriver, currentRate);
}

ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->setSampleRate(theAsioDriver, sampleRate);
}

ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getClockSources(theAsioDriver, clocks, numSources);
}

ASIOError ASIOSetClockSource(long reference) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->setClockSource(theAsioDriver, reference);
}

ASIOError ASIOGetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getSamplePosition(theAsioDriver, sPos, tStamp);
}

ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->getChannelInfo(theAsioDriver, info);
}

ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->createBuffers(theAsioDriver, bufferInfos, numChannels, bufferSize, callbacks);
}

ASIOError ASIODisposeBuffers(void) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->disposeBuffers(theAsioDriver);
}

ASIOError ASIOControlPanel(void) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->controlPanel(theAsioDriver);
}

ASIOError ASIOFuture(long selector, void *params) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->future(theAsioDriver, selector, params);
}

ASIOError ASIOOutputReady(void) {
    if (!theAsioDriver || !theAsioDriver->vtbl)
        return ASE_NotPresent;
    return theAsioDriver->vtbl->outputReady(theAsioDriver);
}

/** @}*/
