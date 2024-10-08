/** @file       cwASIOtypes.h
 *  @brief      cwASIO type definitions
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
    } cwASIOTimeStamp;
    typedef struct cwASIOSamples {
        unsigned long hi;
        unsigned long lo;
    } cwASIOSamples;
#else
#   define NATIVE_INT64 1
    typedef long long int cwASIOTimeStamp;
    typedef long long int cwASIOSamples;
#endif

#define IEEE754_64FLOAT 1
typedef double cwASIOSampleRate;

typedef long cwASIOBool;
enum cwASIObool {
    ASIOFalse = 0,
    ASIOTrue = 1
};

typedef long cwASIOError;
enum cwASIOerror {
    ASE_OK = 0,                 //!< This value will be returned whenever the call succeeded
    ASE_SUCCESS = 0x3f4847a0,   //!< unique success return value for `future()` calls
    ASE_NotPresent = -1000,     //!< hardware input or output is not present or available
    ASE_HWMalfunction,          //!< hardware is malfunctioning (can be returned by any cwASIO function)
    ASE_InvalidParameter,       //!< input parameter invalid
    ASE_InvalidMode,            //!< hardware is in a bad mode or used in a bad mode
    ASE_SPNotAdvancing,         //!< hardware is not running when sample position is inquired
    ASE_NoClock,                //!< sample clock or rate cannot be determined or is not present
    ASE_NoMemory                //!< not enough memory for completing the request
};

typedef long cwASIOSampleType;
enum cwASIOsampleType {
    ASIOSTInt16MSB = 0,
    ASIOSTInt24MSB = 1,         //!< used for 20 bits as well
    ASIOSTInt32MSB = 2,
    ASIOSTFloat32MSB = 3,       //!< IEEE 754 32 bit float
    ASIOSTFloat64MSB = 4,       //!< IEEE 754 64 bit double float

    // these are used for 32 bit data buffer, with different alignment of the data inside
    // 32 bit PCI bus systems can be more easily used with these
    ASIOSTInt32MSB16 = 8,       //!< 32 bit data with 16 bit alignment
    ASIOSTInt32MSB18 = 9,       //!< 32 bit data with 18 bit alignment
    ASIOSTInt32MSB20 = 10,      //!< 32 bit data with 20 bit alignment
    ASIOSTInt32MSB24 = 11,      //!< 32 bit data with 24 bit alignment

    ASIOSTInt16LSB = 16,
    ASIOSTInt24LSB = 17,        //!< used for 20 bits as well
    ASIOSTInt32LSB = 18,
    ASIOSTFloat32LSB = 19,      //!< IEEE 754 32 bit float, as found on Intel x86 architecture
    ASIOSTFloat64LSB = 20,      //!< IEEE 754 64 bit double float, as found on Intel x86 architecture

    // these are used for 32 bit data buffer, with different alignment of the data inside
    // 32 bit PCI bus systems can more easily used with these
    ASIOSTInt32LSB16 = 24,      //!< 32 bit data with 18 bit alignment
    ASIOSTInt32LSB18 = 25,      //!< 32 bit data with 18 bit alignment
    ASIOSTInt32LSB20 = 26,      //!< 32 bit data with 20 bit alignment
    ASIOSTInt32LSB24 = 27,      //!< 32 bit data with 24 bit alignment

    // ASIO DSD format.
    ASIOSTDSDInt8LSB1 = 32,     //!< DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
    ASIOSTDSDInt8MSB1 = 33,     //!< DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
    ASIOSTDSDInt8NER8 = 40,     //!< DSD 8 bit data, 1 sample per byte. No endianness required.

    ASIOSTLastEntry
};

struct cwASIODriverInfo {
    long asioVersion;           //!< currently, 2
    long driverVersion;         //!< driver specific
    char name[32];
    char errorMessage[124];
    void *sysRef;               //!< on input: system reference (on Linux: context pointer)
};

struct cwASIOClockSource {
    long index;                 //!< as used for `setClockSource()`
    long associatedChannel;     //!< for instance, S/P-DIF or AES/EBU
    long associatedGroup;       //!< see channel groups (`getChannelInfo()`)
    long isCurrentSource;       //!< bool; true if this is the current clock source
    char name[32];              //!< for user selection
};

struct cwASIOChannelInfo {
    long channel;               //!< on input, channel index
    cwASIOBool isInput;         //!< on input
    cwASIOBool isActive;        //!< on exit
    long channelGroup;          //!< dto
    cwASIOSampleType type;      //!< dto
    char name[32];              //!< dto
};

struct cwASIOBufferInfo {
    cwASIOBool isInput;         //!< on input:  cwASIOTrue: input, else output
    long channelNum;            //!< on input:  channel index
    void *buffers[2];           //!< on output: double buffer addresses
};

struct cwASIOTimeInfo {
    double speed;               //!< absolute speed (1. = nominal)
    cwASIOTimeStamp systemTime; //!< system time related to samplePosition, in nanoseconds on mac, must be derived from `Microseconds()` (not `UpTime()`!) on windows, must be derived from `timeGetTime()`
    cwASIOSamples samplePosition;
    cwASIOSampleRate sampleRate;//!< current rate
    unsigned long flags;        //!< (see below)
    char reserved[12];
};

enum cwASIOTimeInfoFlags {
    kSystemTimeValid = 1,           //!< must always be valid
    kSamplePositionValid = 1 << 1,  //!< must always be valid
    kSampleRateValid = 1 << 2,
    kSpeedValid = 1 << 3,

    kSampleRateChanged = 1 << 4,
    kClockSourceChanged = 1 << 5
};

struct cwASIOTimeCode {
    double speed;                   //!< speed relation (fraction of nominal speed) optional; set to 0. or 1. if not supported
    cwASIOSamples timeCodeSamples;  //!< time in samples
    unsigned long flags;            //!< some information flags (see below)
    char future[64];
};

enum cwASIOTimeCodeFlags {
    kTcValid = 1,
    kTcRunning = 1 << 1,
    kTcReverse = 1 << 2,
    kTcOnspeed = 1 << 3,
    kTcStill = 1 << 4,

    kTcSpeedValid = 1 << 8
};

struct cwASIOTime {     //!< both input/output
    long reserved[4];               //!< must be 0
    struct cwASIOTimeInfo timeInfo; //!< required
    struct cwASIOTimeCode timeCode; //!< optional, evaluated if (timeCode.flags & kTcValid)
};

struct cwASIOCallbacks {
    /** indicates that both input and output are to be processed.
     * @param doubleBufferIndex the current buffer half index (0 for A, 1 for B).
     * It determines
     * - the output buffer that the host should start to fill. the other buffer
     *   will be passed to output hardware regardless of whether it got filled
     *   in time or not.
     * - the input buffer that is now filled with incoming data. Note that
     *   because of the synchronicity of i/o, the input always has at
     *   least one buffer latency in relation to the output.
     * @param directProcess suggests to the host whether it should immediately
     * start processing (`directProcess == cwASIOTrue`), or whether its process
     * should be deferred because the call comes from a very low level
     * (for instance, a high level priority interrupt), and direct processing
     * would cause timing instabilities for the rest of the system. If in doubt,
     * directProcess should be set to cwASIOFalse.
     * 
     * Note: bufferSwitch may be called at interrupt time for highest efficiency.
     */
    void (*bufferSwitch) (long doubleBufferIndex, cwASIOBool directProcess);

    /** gets called when the AudioStreamIO detects a sample rate change.
     * If sample rate is unknown, 0 is passed (for instance, clock loss
     * when externally synchronized).
     * @param sRate new sample rate
     */
    void (*sampleRateDidChange) (cwASIOSampleRate sRate);

    /** generic callback for various purposes, see `enum cwASIOMessageSel`.
     * note this is only present if the asio version is 2 or higher
     */
    long (*asioMessage) (long selector, long value, void *message, double *opt);

    /** new callback with time info.
     * makes `getSamplePosition()` and various calls to `getSampleRate()` obsolete,
     * and allows for timecode sync etc. to be preferred; will be used if the driver
     * calls `asioMessage()` with selector `kAsioSupportsTimeInfo`.
     */
    struct cwASIOTime *(*bufferSwitchTimeInfo) (struct cwASIOTime *params, long doubleBufferIndex, cwASIOBool directProcess);
};

//! asioMessage selectors
enum cwASIOMessageSel {
    kAsioSelectorSupported = 1, //!< selector in <value>, returns 1L if supported, 0 otherwise
    kAsioEngineVersion,         //!< returns engine (host) asio implementation version, 2 or higher

    /** request driver reset.
     * if accepted, this will close the driver (`ASIOExit()`) and re-open it again (`init()` etc).
     * some drivers need to reconfigure for instance when the sample rate changes, or some basic
     * changes have been made in `controlPanel()`. returns 1L; note the request is merely passed
     * to the application, there is no way to determine if it gets accepted at this time (but it
     * usually will be).
     */
    kAsioResetRequest,

    /** not yet supported, will currently always return 0L.
     * for now, use `kAsioResetRequest` instead.
     * once implemented, the new buffer size is expected in <value>, and on success returns 1L
     */
    kAsioBufferSizeChange,

    /** the driver went out of sync, such that the timestamp is no longer valid.
     * this is a request to re-start the engine and slave devices (sequencer).
     * returns 1 for ok, 0 if not supported.
     */
    kAsioResyncRequest,

    kAsioLatenciesChanged,      //!< the drivers latencies have changed. The engine will refetch the latencies.
    kAsioSupportsTimeInfo,      //!< if host returns true here, it will expect `bufferSwitchTimeInfo()` to be called instead of `bufferSwitch()`
    kAsioSupportsTimeCode,
    kAsioMMCCommand,            //!< unused - value: number of commands, message points to mmc commands
    kAsioSupportsInputMonitor,  //!< `kAsioSupportsXXX` return 1 if host supports this
    kAsioSupportsInputGain,     //!< unused and undefined
    kAsioSupportsInputMeter,    //!< unused and undefined
    kAsioSupportsOutputGain,    //!< unused and undefined
    kAsioSupportsOutputMeter,   //!< unused and undefined
    kAsioOverload,              //!< driver detected an overload

    kAsioNumMessageSelectors
};

enum cwASIOFutureSel {
    kAsioEnableTimeCodeRead = 1,    //!< no arguments
    kAsioDisableTimeCodeRead,       //!< no arguments
    kAsioSetInputMonitor,           //!< cwASIOInputMonitor* in params
    kAsioTransport,                 //!< cwASIOTransportParameters* in params
    kAsioSetInputGain,              //!< cwASIOChannelControls* in params, apply gain
    kAsioGetInputMeter,             //!< cwASIOChannelControls* in params, fill meter
    kAsioSetOutputGain,             //!< cwASIOChannelControls* in params, apply gain
    kAsioGetOutputMeter,            //!< cwASIOChannelControls* in params, fill meter
    kAsioCanInputMonitor,           //!< no arguments
    kAsioCanTimeInfo,               //!< no arguments
    kAsioCanTimeCode,               //!< no arguments
    kAsioCanTransport,              //!< no arguments
    kAsioCanInputGain,              //!< no arguments
    kAsioCanInputMeter,             //!< no arguments
    kAsioCanOutputGain,             //!< no arguments
    kAsioCanOutputMeter,            //!< no arguments
    kAsioOptionalOne,

    // DSD support
    // The following extensions are required to allow switching and control of the DSD subsystem.
    kAsioSetIoFormat = 0x23111961,      //!< cwASIOIoFormat * in params.
    kAsioGetIoFormat = 0x23111983,      //!< cwASIOIoFormat * in params.
    kAsioCanDoIoFormat = 0x23112004,    //!< cwASIOIoFormat * in params.

    // Extension for drop out detection
    kAsioCanReportOverload = 0x24042012,    //!< return `ASE_SUCCESS` if driver can detect and report overloads

    kAsioGetInternalBufferSamples = 0x25042012,  //!< cwASIOInternalBufferInfo * in params. Deliver size of driver internal buffering, return `ASE_SUCCESS` if supported

    // cwASIO extensions
    kcwASIOsetInstanceName = 0x7F000001   //!< char const * to name in params
};

struct cwASIOInputMonitor {
    long input;         //!< this input was set to monitor (or off), -1: all
    long output;        //!< suggested output for monitoring the input (if so)
    long gain;          //!< suggested gain, ranging 0 - 0x7fffffffL (-inf to +12 dB)
    cwASIOBool state;   //!< cwASIOTrue => on, cwASIOFalse => off
    long pan;           //!< suggested pan, 0 => all left, 0x7fffffff => right
};

struct cwASIOChannelControls {
    long channel;       //!< on input, channel index
    cwASIOBool isInput; //!< on input
    long gain;          //!< on input,  ranges 0 thru 0x7fffffff
    long meter;         //!< on return, ranges 0 thru 0x7fffffff
    char future[32];
};

struct cwASIOTransportParameters {
    long command;       // see enum below
    cwASIOSamples samplePosition;
    long track;
    long trackSwitches[16]; //!< 512 tracks on/off
    char future[64];
};

enum cwASIOtransportCmd {
    kTransStart = 1,
    kTransStop,
    kTransLocate,       //!< to samplePosition
    kTransPunchIn,
    kTransPunchOut,
    kTransArmOn,        //!< track
    kTransArmOff,       //!< track
    kTransMonitorOn,    //!< track
    kTransMonitorOff,   //!< track
    kTransArm,          //!< trackSwitches
    kTransMonitor       //!< trackSwitches
};

typedef long int ASIOIoFormatType;
enum cwASIOIoFormatType {
    kASIOFormatInvalid = -1,
    kASIOPCMFormat = 0,
    kASIODSDFormat = 1,
};

struct cwASIOIoFormat {
    ASIOIoFormatType    FormatType;
    char                future[512 - sizeof(ASIOIoFormatType)];
};

/** Extension for drop detection.
 * Note: Refers to buffering that goes beyond the double buffer e.g. used by USB driver designs
 */
struct cwASIOInternalBufferInfo {
    long inputSamples;  //!< size of driver's internal input buffering which is included in getLatencies
    long outputSamples; //!< size of driver's internal output buffering which is included in getLatencies
};

/** @}*/
