/** @file       asio.h
 *  @brief      ASIO compatibility header for cwASIO
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */
#pragma once

#include "cwASIO.h"

/** Load the driver and enable the below functions.
* If you want to use the C functions below, which are ASIO SDK compatible, then
* you need to use ASIOLoad() to load the driver, instead of using cwASIOload().
* This causes a reference to the driver being remembered in a global variable
* behind the scenes.
* @param key A system-specific textual key that identifies the driver to load.
* @return an error code, which is zero on success.
*/
ASIOError ASIOLoad(char const *key);

/** Unload the driver and disable the below functions.
* @return an error code, which is zero on success.
*/
ASIOError ASIOUnload(void);

ASIOError ASIOInit(ASIODriverInfo *info);
ASIOError ASIOExit(void);
ASIOError ASIOStart(void);
ASIOError ASIOStop(void);
ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels);
ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency);
ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity);
ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate);
ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate);
ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate);
ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources);
ASIOError ASIOSetClockSource(long reference);
ASIOError ASIOGetSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp);
ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info);
ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks);
ASIOError ASIODisposeBuffers(void);
ASIOError ASIOControlPanel(void);
ASIOError ASIOFuture(long selector, void *params);
ASIOError ASIOOutputReady(void);

/** @}*/
