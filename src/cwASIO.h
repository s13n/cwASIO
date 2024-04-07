/** @file       cwASIO.h
 *  @brief      cwASIO API
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */
#pragma once

#ifdef _WIN32
#   define NATIVE_INT64 0
    typedef struct cwASIOTimeStamp {
        unsigned long hi;
        unsigned long lo;
    } ASIOTimeStamp;
    typedef struct cwASIOSamples {
        unsigned long hi;
        unsigned long lo;
    } ASIOSamples;
#else
#   define NATIVE_INT64 1
    typedef long long int ASIOTimeStamp;
    typedef long long int ASIOSamples;
#endif

#define IEEE754_64FLOAT 1
typedef double ASIOSampleRate;

typedef long ASIOBool;
enum cwASIOBool {
    ASIOFalse = 0,
    ASIOTrue = 1
};

typedef long ASIOError;
enum cwASIOerror {
    ASE_OK = 0,             // This value will be returned whenever the call succeeded
    ASE_SUCCESS = 0x3f4847a0, // unique success return value for ASIOFuture calls
    ASE_NotPresent = -1000, // hardware input or output is not present or available
    ASE_HWMalfunction,      // hardware is malfunctioning (can be returned by any ASIO function)
    ASE_InvalidParameter,   // input parameter invalid
    ASE_InvalidMode,        // hardware is in a bad mode or used in a bad mode
    ASE_SPNotAdvancing,     // hardware is not running when sample position is inquired
    ASE_NoClock,            // sample clock or rate cannot be determined or is not present
    ASE_NoMemory            // not enough memory for completing the request
};

typedef long ASIOSampleType;
enum cwASIOSampleType {
    ASIOSTInt16MSB   = 0,
    ASIOSTInt24MSB   = 1,       // used for 20 bits as well
    ASIOSTInt32MSB   = 2,
    ASIOSTFloat32MSB = 3,       // IEEE 754 32 bit float
    ASIOSTFloat64MSB = 4,       // IEEE 754 64 bit double float

    // these are used for 32 bit data buffer, with different alignment of the data inside
    // 32 bit PCI bus systems can be more easily used with these
    ASIOSTInt32MSB16 = 8,       // 32 bit data with 16 bit alignment
    ASIOSTInt32MSB18 = 9,       // 32 bit data with 18 bit alignment
    ASIOSTInt32MSB20 = 10,      // 32 bit data with 20 bit alignment
    ASIOSTInt32MSB24 = 11,      // 32 bit data with 24 bit alignment

    ASIOSTInt16LSB   = 16,
    ASIOSTInt24LSB   = 17,      // used for 20 bits as well
    ASIOSTInt32LSB   = 18,
    ASIOSTFloat32LSB = 19,      // IEEE 754 32 bit float, as found on Intel x86 architecture
    ASIOSTFloat64LSB = 20,      // IEEE 754 64 bit double float, as found on Intel x86 architecture

    // these are used for 32 bit data buffer, with different alignment of the data inside
    // 32 bit PCI bus systems can more easily used with these
    ASIOSTInt32LSB16 = 24,      // 32 bit data with 18 bit alignment
    ASIOSTInt32LSB18 = 25,      // 32 bit data with 18 bit alignment
    ASIOSTInt32LSB20 = 26,      // 32 bit data with 20 bit alignment
    ASIOSTInt32LSB24 = 27,      // 32 bit data with 24 bit alignment

    // ASIO DSD format.
    ASIOSTDSDInt8LSB1 = 32,     // DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
    ASIOSTDSDInt8MSB1 = 33,     // DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
    ASIOSTDSDInt8NER8 = 40,     // DSD 8 bit data, 1 sample per byte. No Endianness required.

    ASIOSTLastEntry
};

typedef enum cwAsioTimeInfoFlags {
    kSystemTimeValid        = 1,            // must always be valid
    kSamplePositionValid    = 1 << 1,       // must always be valid
    kSampleRateValid        = 1 << 2,
    kSpeedValid             = 1 << 3,

    kSampleRateChanged      = 1 << 4,
    kClockSourceChanged     = 1 << 5
} AsioTimeInfoFlags;

typedef struct cwASIODriverInfo {
    long asioVersion;       // currently, 2
    long driverVersion;     // driver specific
    char name[32];
    char errorMessage[124];
    void *sysRef;           // on input: system reference (on Linux: context pointer)
} ASIODriverInfo;

typedef struct cwASIOClockSource {
    long index;             // as used for setClockSource()
    long associatedChannel; // for instance, S/P-DIF or AES/EBU
    long associatedGroup;   // see channel groups (getChannelInfo())
    long isCurrentSource;   // bool; true if this is the current clock source
    char name[32];          // for user selection
} ASIOClockSource;

typedef struct cwASIOChannelInfo {
    long channel;           // on input, channel index
    ASIOBool isInput;       // on input
    ASIOBool isActive;      // on exit
    long channelGroup;      // dto
    ASIOSampleType type;    // dto
    char name[32];          // dto
} ASIOChannelInfo;

typedef struct cwASIOBufferInfo {
    ASIOBool isInput;       // on input:  ASIOTrue: input, else output
    long channelNum;        // on input:  channel index
    void *buffers[2];       // on output: double buffer addresses
} ASIOBufferInfo;

typedef struct cwAsioTimeInfo {
    double speed;               // absolute speed (1. = nominal)
    ASIOTimeStamp systemTime;   // system time related to samplePosition, in nanoseconds
                                // on mac, must be derived from Microseconds() (not UpTime()!)
                                // on windows, must be derived from timeGetTime()
    ASIOSamples samplePosition;
    ASIOSampleRate sampleRate;  // current rate
    unsigned long flags;        // (see below)
    char reserved[12];
} AsioTimeInfo;

typedef struct cwASIOTimeCode {
    double speed;               // speed relation (fraction of nominal speed)
                                // optional; set to 0. or 1. if not supported
    ASIOSamples timeCodeSamples;// time in samples
    unsigned long flags;        // some information flags (see below)
    char future[64];
} ASIOTimeCode;

typedef struct cwASIOTime {     // both input/output
    long reserved[4];           // must be 0
    AsioTimeInfo timeInfo;      // required
    ASIOTimeCode timeCode;      // optional, evaluated if (timeCode.flags & kTcValid)
} ASIOTime;

typedef struct cwASIOCallbacks {
    void (*bufferSwitch) (long doubleBufferIndex, ASIOBool directProcess);
        // bufferSwitch indicates that both input and output are to be processed.
        // the current buffer half index (0 for A, 1 for B) determines
        // - the output buffer that the host should start to fill. the other buffer
        //   will be passed to output hardware regardless of whether it got filled
        //   in time or not.
        // - the input buffer that is now filled with incoming data. Note that
        //   because of the synchronicity of i/o, the input always has at
        //   least one buffer latency in relation to the output.
        // directProcess suggests to the host whether it should immedeately
        // start processing (directProcess == ASIOTrue), or whether its process
        // should be deferred because the call comes from a very low level
        // (for instance, a high level priority interrupt), and direct processing
        // would cause timing instabilities for the rest of the system. If in doubt,
        // directProcess should be set to ASIOFalse.
        // Note: bufferSwitch may be called at interrupt time for highest efficiency.

    void (*sampleRateDidChange) (ASIOSampleRate sRate);
        // gets called when the AudioStreamIO detects a sample rate change
        // If sample rate is unknown, 0 is passed (for instance, clock loss
        // when externally synchronized).

    long (*asioMessage) (long selector, long value, void *message, double *opt);
        // generic callback for various purposes, see selectors below.
        // note this is only present if the asio version is 2 or higher

    ASIOTime *(*bufferSwitchTimeInfo) (ASIOTime *params, long doubleBufferIndex, ASIOBool directProcess);
        // new callback with time info. makes ASIOGetSamplePosition() and various
        // calls to ASIOGetSampleRate obsolete,
        // and allows for timecode sync etc. to be preferred; will be used if
        // the driver calls asioMessage with selector kAsioSupportsTimeInfo.
} ASIOCallbacks;

/** Load the driver and enable the below functions.
* If you want to use the C functions below, which are ASIO SDK compatible, then
* you need to use ASIOLoad() to load the driver, instead of using cwASIOload().
* This causes a reference to the driver being remembered in a global variable
* behind the scenes.
* @param key A system-specific textual key that identifies the driver to load.
* @return an error code, which is zero on success.
*/
ASIOError ASIOLoad(char const *key);

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
ASIOError ASIOGetSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp);
ASIOError ASIOGetChannelInfo(ASIOChannelInfo *info);
ASIOError ASIOCreateBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks const *callbacks);
ASIOError ASIODisposeBuffers(void);
ASIOError ASIOControlPanel(void);
ASIOError ASIOFuture(long selector, void *params);
ASIOError ASIOOutputReady(void);

/** @}*/
