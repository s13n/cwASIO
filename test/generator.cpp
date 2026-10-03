/** @file       generator.cpp
 *  @brief      cwASIO playback application
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO_test
 *  @{
 */

#include "cwASIO.hpp"
#include <bit>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::literals;

static_assert(std::endian::native == std::endian::little);

static const int32_t sampleValue32Mask24 = 0xFFFFFF00;
std::vector<int32_t> lastSampleValues32;
unsigned sampleBufferIndex = 0;
cwASIOSampleType sampleType = ASIOSTLastEntry;
static long numberOfChannels = 0;
static std::vector<cwASIOBufferInfo> bufferInfos;
static long blocksize = 0;
static std::vector<cwASIOChannelInfo> channelInfos;
static std::sig_atomic_t volatile signalStatus = 0;
static std::sig_atomic_t volatile stopStatus = 0;


static void signalHandler(int signal) {
    signalStatus = signal;
}

// creating a 24 bit ramp overflowing from max 24 bit val to min 24 bit val
static inline int32_t nextSampleValue32(int32_t currentSampleValue32) {
    if(currentSampleValue32 == sampleValue32Mask24) {
        int i; i = 0;
    }
    return (currentSampleValue32 + 256) & sampleValue32Mask24;
}

static void bufferSwitch(long doubleBufferIndex, cwASIOBool directProcess) {
    for(long channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex) {
        int32_t *s = static_cast<int32_t *>(bufferInfos[channelIndex].buffers[doubleBufferIndex]);
        for(long i = 0; i < blocksize; ++i, ++s) {
            lastSampleValues32[channelIndex] = *s = nextSampleValue32(lastSampleValues32[channelIndex]);
        }
    }
}

static void sampleRateDidChange(cwASIOSampleRate sRate) {
}

static long asioMessage(long selector, long value, void *message, double *opt) {
    return 0;
}

static struct cwASIOTime *bufferSwitchTimeInfo(struct cwASIOTime *params, long doubleBufferIndex, cwASIOBool directProcess) {
    bufferSwitch(doubleBufferIndex, directProcess);
    return params;
}

static cwASIOCallbacks const callbacks = {
    .bufferSwitch = &bufferSwitch,
    .sampleRateDidChange = &sampleRateDidChange,
    .asioMessage = &asioMessage,
    .bufferSwitchTimeInfo = &bufferSwitchTimeInfo
};

int main(int argc, char const *argv[]) {
    if(argc != 4) {
        std::cout << "Usage: player <ASIO device> <first channel index> <number of channels>\n";
        return 1;
    }

    try {
        std::error_code ec;
        cwASIO::Device driver(argv[1]);
        auto firstChanIndex = strtol(argv[2], nullptr, 10);
        if(firstChanIndex < 0)
            throw std::runtime_error(std::format("Invalid first channel index given: {}"
                , firstChanIndex));
        auto numChans = strtol(argv[3], nullptr, 10);
        if(numChans <= 0)
            throw std::runtime_error(std::format("Invalid number of channels given: {}"
                , numChans));

        // tell the cwASIO driver that we are a modern app (knowing about multi instance drivers)
        if(driver.future(kcwASIOsetInstanceName, (void*) argv[1]))
            std::cout << "The chosen cwASIO driver \"" << argv[1] << "\" does NOT support multiple instances!\n";

        cwASIODriverInfo driverinfo = driver.init(nullptr);
        if(driverinfo.errorMessage[0] != '\0')
            throw std::runtime_error(std::format("Can't init driver {} version {}: {}"
                    , driverinfo.name, driverinfo.driverVersion, driverinfo.errorMessage));

        std::cout << "Initialised cwASIO driver \"" << argv[1] << "\" (\"" << driverinfo.name << "\").\n";

        auto [_, numOutputChannels] = driver.getChannels(ec);
        if(ec)
            throw std::system_error(ec, "when reading number of channels");
        if(firstChanIndex + numChans > numOutputChannels)
            throw std::runtime_error("not enough output channels");

        auto [_0, _1, preferredSize, _2] = driver.getBufferSize(ec);
        if(ec)
            throw std::system_error(ec, "when reading supported buffer sizes");

        uint32_t samplerate = uint32_t(driver.getSampleRate(ec));
        if(ec)
            throw std::system_error(ec, "when reading sampling rate");

        bufferInfos.resize(size_t(numChans));
        for(long channelIndex = 0; channelIndex < numChans; channelIndex++) {
            bufferInfos[channelIndex].isInput = false;
            bufferInfos[channelIndex].channelNum = firstChanIndex + channelIndex;
        }
        if(auto err = driver.createBuffers(bufferInfos.data(), bufferInfos.size(), preferredSize, &callbacks))
            throw std::system_error(err, cwASIO::err_category(), "when trying to create the buffers");
        blocksize = preferredSize;

        sampleType = ASIOSTInt32LSB;
        channelInfos.resize(size_t(numChans));
        for(long ch = 0; ch < long(std::size(channelInfos)); ++ch) {
            channelInfos[ch].channel = firstChanIndex + ch;
            channelInfos[ch].isInput = false;
            if(auto err = driver.getChannelInfo(channelInfos[ch]))
                throw std::system_error(err, cwASIO::err_category(), "when reading the info for channel with index " + std::to_string(ch));
            if(channelInfos[ch].type != sampleType)
                throw std::runtime_error("Sample type not supported on channel with index " + std::to_string(ch) + " (" + channelInfos[ch].name + ")");
        }

        lastSampleValues32.resize(size_t(numChans));
        for(auto &lastSampleValue32 : lastSampleValues32) {
            lastSampleValue32 = sampleValue32Mask24;
        }
        numberOfChannels = numChans;

        std::signal(SIGINT, signalHandler);

        std::cout << "Now starting generating signal\n";

        if(auto err = driver.start())
            throw std::system_error(err, cwASIO::err_category(), "when trying to start streaming");

        std::cout << "Generating on playback device " << driver.getDriverName()
            << " (" << channelInfos[0].name << "/" << channelInfos[numChans-1].name << ") at " << samplerate << " Hz\n";

        while(signalStatus == 0 && stopStatus == 0)
            std::this_thread::sleep_for(10ms);
        if (signalStatus != 0)
            printf("\ngenerating aborted\n");
    } catch(std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}
