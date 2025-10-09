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


static struct cwASIODriver *theAsioDriver = NULL;

ASIOError ASIOLoad(char const *id, char const *name) {
    if(theAsioDriver)
        return ASE_NoMemory;
    ASIOError err = cwASIOload(id, &theAsioDriver);
    if(err != ASE_OK && err != ASE_SUCCESS) {
        theAsioDriver = NULL;
        return err;
    }
    err = theAsioDriver->lpVtbl->future(theAsioDriver, kcwASIOsetInstanceName, (void *)name);
    return err == ASE_SUCCESS || err == ASE_InvalidParameter ? ASE_OK : err;
}

ASIOError ASIOUnload(void) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_InvalidParameter;
    cwASIOunload(theAsioDriver);
    theAsioDriver = NULL;
    return ASE_OK;
}

ASIOError ASIOInit(ASIODriverInfo *info) {
    if(!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    if (!theAsioDriver->lpVtbl->init(theAsioDriver, info ? info->sysRef : 0))
        return ASE_NotPresent;
    if(info) {
        info->asioVersion = 2;
        theAsioDriver->lpVtbl->getDriverName(theAsioDriver, info->name);
        info->driverVersion = theAsioDriver->lpVtbl->getDriverVersion(theAsioDriver);
        theAsioDriver->lpVtbl->getErrorMessage(theAsioDriver, info->errorMessage);
    }
    return ASE_OK;
}

ASIOError ASIOExit(void) {
    return ASE_OK;
}

ASIOError ASIOStart(void) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->start(theAsioDriver);
}

ASIOError ASIOStop(void) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->stop(theAsioDriver);
}

ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getChannels(theAsioDriver, numInputChannels, numOutputChannels);
}

ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getLatencies(theAsioDriver, inputLatency, outputLatency);
}

ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getBufferSize(theAsioDriver, minSize, maxSize, preferredSize, granularity);
}

ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->canSampleRate(theAsioDriver, sampleRate);
}

ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getSampleRate(theAsioDriver, currentRate);
}

ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->setSampleRate(theAsioDriver, sampleRate);
}

ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getClockSources(theAsioDriver, clocks, numSources);
}

ASIOError ASIOSetClockSource(long reference) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->setClockSource(theAsioDriver, reference);
}

ASIOError ASIOGetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getSamplePosition(theAsioDriver, sPos, tStamp);
}

ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->getChannelInfo(theAsioDriver, info);
}

ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->createBuffers(theAsioDriver, bufferInfos, numChannels, bufferSize, callbacks);
}

ASIOError ASIODisposeBuffers(void) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->disposeBuffers(theAsioDriver);
}

ASIOError ASIOControlPanel(void) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->controlPanel(theAsioDriver);
}

ASIOError ASIOFuture(long selector, void *params) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->future(theAsioDriver, selector, params);
}

ASIOError ASIOOutputReady(void) {
    if (!theAsioDriver || !theAsioDriver->lpVtbl)
        return ASE_NotPresent;
    return theAsioDriver->lpVtbl->outputReady(theAsioDriver);
}

/** @}*/
