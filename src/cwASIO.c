/** @file       cwASIO.c
 *  @brief      cwASIO API (Unix implementation)
 *  @author     Axel Holzinger
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2014-2024
 *  @copyright  Usage and copying requires a license granted by the authors
 * @addtogroup AsioDevice
 *  @{
 */

#include "cwASIOifc.h"
#include <alloca.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static struct AsioDriver *theAsioDriver = 0;
static struct cwASIO_DriverInterface driverIfc = { 0 };


ASIOError ASIOLoad(char const *path) {
    if(driverIfc.driverLib)
        return ASE_NoMemory;
    return cwASIOload(path, &driverIfc);
}

ASIOError ASIOInit(ASIODriverInfo *info) {
    if(theAsioDriver || !driverIfc.driverLib)
        return ASE_NotPresent;
    theAsioDriver = driverIfc.instantiate(info ? info->sysRef : 0);
    if(!theAsioDriver)
        return ASE_NotPresent;
    if(info) {
        info->asioVersion = 2;
        driverIfc.getDriverName(theAsioDriver, info->name);
        info->driverVersion = driverIfc.getDriverVersion(theAsioDriver);
        driverIfc.getErrorMessage(theAsioDriver, info->errorMessage);
    }
    return ASE_OK;
}

ASIOError ASIOExit(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    driverIfc.discard(theAsioDriver);
    theAsioDriver = 0;
    return ASE_OK;
}

ASIOError ASIOStart(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.start(theAsioDriver);
}

ASIOError ASIOStop(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.stop(theAsioDriver);
}

ASIOError ASIOGetChannels(long *numInputChannels, long *numOutputChannels) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getChannels(theAsioDriver, numInputChannels, numOutputChannels);
}

ASIOError ASIOGetLatencies(long *inputLatency, long *outputLatency) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getLatencies(theAsioDriver, inputLatency, outputLatency);
}

ASIOError ASIOGetBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getBufferSize(theAsioDriver, minSize, maxSize, preferredSize, granularity);
}

ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.canSampleRate(theAsioDriver, sampleRate);
}

ASIOError ASIOGetSampleRate(ASIOSampleRate *currentRate) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getSampleRate(theAsioDriver, currentRate);
}

ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.setSampleRate(theAsioDriver, sampleRate);
}

ASIOError ASIOGetClockSources(ASIOClockSource *clocks, long *numSources) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getClockSources(theAsioDriver, clocks, numSources);
}

ASIOError ASIOSetClockSource(long reference) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.setClockSource(theAsioDriver, reference);
}

ASIOError ASIOGetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getSamplePosition(theAsioDriver, sPos, tStamp);
}

ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.getChannelInfo(theAsioDriver, info);
}

ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.createBuffers(theAsioDriver, bufferInfos, numChannels, bufferSize, callbacks);
}

ASIOError ASIODisposeBuffers(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.disposeBuffers(theAsioDriver);
}typedef bool (EnumerateCallback)(void *, char const *, char const *, char const *);

int cwASIOenumerate(EnumerateCallback *cb, void *context);



ASIOError ASIOControlPanel(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.controlPanel(theAsioDriver);
}

ASIOError ASIOFuture(long selector, void *params) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.future(theAsioDriver, selector, params);
}

ASIOError ASIOOutputReady(void) {
    if(!theAsioDriver)
        return ASE_NotPresent;
    return driverIfc.outputReady(theAsioDriver);
}


long cwASIOload(char const *path, struct cwASIO_DriverInterface *ifc) {
    void *lib = dlopen(path, RTLD_LOCAL | RTLD_NOW);
    if(!lib)
        return ASE_NotPresent;
    ifc->instantiate = (struct AsioDriver *(*)(void *))dlsym(lib, "cwASIOinstantiate");
    if(!ifc->instantiate)
        return ASE_NotPresent;
    ifc->discard = (void (*)(struct AsioDriver *))dlsym(lib, "cwASIOdiscard");
    if(!ifc->discard)
        return ASE_NotPresent;
    ifc->getDriverName = (void (*)(struct AsioDriver *, char *))dlsym(lib, "cwASIOgetDriverName");
    if(!ifc->getDriverName)
        return ASE_NotPresent;
    ifc->getDriverVersion = (long (*)(struct AsioDriver *))dlsym(lib, "cwASIOgetDriverVersion");
    if(!ifc->getDriverVersion)
        return ASE_NotPresent;
    ifc->getErrorMessage = (void (*)(struct AsioDriver *, char *))dlsym(lib, "cwASIOgetErrorMessage");
    if(!ifc->getErrorMessage)
        return ASE_NotPresent;
    ifc->start = (long (*)(struct AsioDriver *))dlsym(lib, "cwASIOstart");
    if(!ifc->start)
        return ASE_NotPresent;
    ifc->stop = (long (*)(struct AsioDriver *))dlsym(lib, "cwASIOstop");
    if(!ifc->stop)
        return ASE_NotPresent;
    ifc->getChannels = (long (*)(struct AsioDriver *, long *, long *))dlsym(lib, "cwASIOgetChannels");
    if(!ifc->getChannels)
        return ASE_NotPresent;
    ifc->getLatencies = (long (*)(struct AsioDriver *, long *, long *))dlsym(lib, "cwASIOgetLatencies");
    if(!ifc->getLatencies)
        return ASE_NotPresent;
    ifc->getBufferSize = (long (*)(struct AsioDriver *, long *, long *, long *, long *))dlsym(lib, "cwASIOgetBufferSize");
    if(!ifc->getBufferSize)
        return ASE_NotPresent;
    ifc->canSampleRate = (long (*)(struct AsioDriver *, double))dlsym(lib, "cwASIOcanSampleRate");
    if(!ifc->canSampleRate)
        return ASE_NotPresent;
    ifc->getSampleRate = (long (*)(struct AsioDriver *, double *))dlsym(lib, "cwASIOgetSampleRate");
    if(!ifc->getSampleRate)
        return ASE_NotPresent;
    ifc->setSampleRate = (long (*)(struct AsioDriver *, double))dlsym(lib, "cwASIOsetSampleRate");
    if(!ifc->setSampleRate)
        return ASE_NotPresent;
    ifc->getClockSources = (long (*)(struct AsioDriver *, ASIOClockSource *, long *))dlsym(lib, "cwASIOgetClockSources");
    if(!ifc->getClockSources)
        return ASE_NotPresent;
    ifc->setClockSource = (long (*)(struct AsioDriver *, long))dlsym(lib, "cwASIOsetClockSource");
    if(!ifc->setClockSource)
        return ASE_NotPresent;
    ifc->getSamplePosition = (long (*)(struct AsioDriver *, ASIOSamples *, ASIOTimeStamp *))dlsym(lib, "cwASIOgetSamplePosition");
    if(!ifc->getSamplePosition)
        return ASE_NotPresent;
    ifc->getChannelInfo = (long (*)(struct AsioDriver *, ASIOChannelInfo *))dlsym(lib, "cwASIOgetChannelInfo");
    if(!ifc->getChannelInfo)
        return ASE_NotPresent;
    ifc->createBuffers = (long (*)(struct AsioDriver *, ASIOBufferInfo *, long , long , ASIOCallbacks const *))dlsym(lib, "cwASIOcreateBuffers");
    if(!ifc->createBuffers)
        return ASE_NotPresent;
    ifc->disposeBuffers = (long (*)(struct AsioDriver *))dlsym(lib, "cwASIOdisposeBuffers");
    if(!ifc->disposeBuffers)
        return ASE_NotPresent;
    ifc->controlPanel = (long (*)(struct AsioDriver *))dlsym(lib, "cwASIOcontrolPanel");
    if(!ifc->controlPanel)
        return ASE_NotPresent;
    ifc->future = (long (*)(struct AsioDriver *, long, void *))dlsym(lib, "cwASIOfuture");
    if(!ifc->future)
        return ASE_NotPresent;
    ifc->outputReady = (long (*)(struct AsioDriver *))dlsym(lib, "cwASIOoutputReady");
    if(!ifc->outputReady)
        return ASE_NotPresent;
    ifc->driverLib = lib;
    return ASE_OK;
}

void cwASIOunload(struct cwASIO_DriverInterface *ifc) {
    if(ifc)
        dlclose(ifc->driverLib);
}


static char *cwASIOreadConfig(char const *base, char const *name, char const *file) {
    size_t baseLen = strlen(base);
    size_t nameLen = strlen(name);
    char *path = (char*)alloca(baseLen + 1 + nameLen + strlen(file) + 1);
    strcpy(path, base);
    path[baseLen] = '/';
    strcpy(path + baseLen + 1, name);
    strcpy(path + baseLen + 1 + nameLen, file);

    int fd = open(path, O_RDONLY);
    if(fd < 0)
        return NULL;

    struct stat st;
    if(fstat(fd, &st) < 0) {
        int err = errno;
        close(fd);
        errno = err;
        return NULL;
    }

    if(st.st_size > 1023)
        st.st_size = 1023;  // limit buffer size
    char *txt = (char*)malloc(st.st_size + 1);
    ssize_t len = read(fd, txt, st.st_size);
    int err = errno;
    close(fd);
    if(len < 0) {
        free(txt);
        errno = err;
        return NULL;
    } else {
        txt[len] = '\0';
        char *end = strchr(txt, '\n');
        if(end)
            *end = '\0';    // terminate at end of first line
        errno = err;
        return txt;
    }
}

int cwASIOenumerate(EnumerateCallback *cb, void *context) {
    static char const *path = "/etc/cwASIO";
    int res = 0;
    struct dirent *rent;
    DIR *base = opendir(path);

    if(!base)
        return errno;

    do {
        errno = 0;
        rent = readdir(base);
        if(!rent) {
            res = errno;
            break;
        } else if(rent->d_name[0] == '.') {
            continue;   // ignore entries starting with a dot
        } else {
            char *driver = cwASIOreadConfig(path, rent->d_name, "/driver");
            char *description = cwASIOreadConfig(path, rent->d_name, "/description");
            if(!cb(context, rent->d_name, driver, description))
                rent = NULL;
            free(driver);
            free(description);
        }
    } while(rent);

    closedir(base);
    return res;
}

/** @}*/
