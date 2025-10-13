/** @file       player.cpp
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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "wavefile/wavefile.hpp"

using namespace std::literals;

static_assert(std::endian::native == std::endian::little);

std::vector<int16_t> fileBuffer16;
std::vector<int32_t> fileBuffer32;
unsigned fileBufferIndex = 0;
cwASIOSampleType sampleType = ASIOSTLastEntry;
static std::vector<cwASIOBufferInfo> bufferInfos(2);
static long blocksize = 0;
static cwASIOChannelInfo channelInfos[2];
static std::sig_atomic_t volatile signalStatus = 0;
static std::sig_atomic_t volatile stopStatus = 0;


static void signalHandler(int signal) {
    signalStatus = signal;
}

static void bufferSwitch(long doubleBufferIndex, cwASIOBool directProcess) {
    if (sampleType == ASIOSTInt32LSB) {
        auto leftCh  = static_cast<int32_t *>(bufferInfos[0].buffers[doubleBufferIndex]);
        auto rightCh = static_cast<int32_t *>(bufferInfos[1].buffers[doubleBufferIndex]);
        for(long i = 0; i < blocksize; ++i) {
            if (fileBufferIndex < fileBuffer32.size()) {
                *leftCh++  = fileBuffer32.at(fileBufferIndex++);
                *rightCh++ = fileBuffer32.at(fileBufferIndex++);
            } else {
                stopStatus = 1;
                *leftCh++  = 0;
                *rightCh++ = 0;
            }
        }
    } else {
        auto leftCh  = static_cast<int16_t *>(bufferInfos[0].buffers[doubleBufferIndex]);
        auto rightCh = static_cast<int16_t *>(bufferInfos[1].buffers[doubleBufferIndex]);
        for(long i = 0; i < blocksize; ++i) {
            if (fileBufferIndex < fileBuffer16.size()) {
                *leftCh++  = fileBuffer16.at(fileBufferIndex++);
                *rightCh++ = fileBuffer16.at(fileBufferIndex++);
            } else {
                stopStatus = 1;
                *leftCh++  = 0;
                *rightCh++ = 0;
            }
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

static bool hasSupportedSampleFormat(WaveFile const &file) {
    if(file.getBitsPerSample() == 32 && file.getBytesPerSample() == 4)
        return true;
    if(file.getBitsPerSample() == 16 && file.getBytesPerSample() == 2)
        return true;
    return false;
}

int main(int argc, char const *argv[]) {
    if(argc != 4) {
        std::cout << "Usage: player <ASIO device> <first channel index> <filename>\n";
        return 1;
    }

    try {
        std::error_code ec;
        cwASIO::Device driver(argv[1]);
        auto firstChanIndex = strtol(argv[2], nullptr, 10);
        std::filesystem::path filepath(argv[3]);

        // tell the cwASIO driver that we are a modern app (knowing about multi instance drivers)
        if(driver.future(kcwASIOsetInstanceName, (void*) argv[1]))
            std::cout << "The chosen cwASIO driver \"" << argv[1] << "\" does NOT support multiple instances!\n";

        if(!driver.init(nullptr))
            throw std::runtime_error("Can't init driver " + driver.getDriverName() + " version "
                    + std::to_string(driver.getDriverVersion()) + ": " + driver.getErrorMessage());

        auto [_, numOutputChannels] = driver.getChannels(ec);
        if(ec)
            throw std::system_error(ec, "when reading number of channels");
        if(firstChanIndex + 2 > numOutputChannels)
            throw std::runtime_error("not enough output channels");

        auto [_0, _1, preferredSize, _2] = driver.getBufferSize(ec);
        if(ec)
            throw std::system_error(ec, "when reading supported buffer sizes");

        uint32_t samplerate = uint32_t(driver.getSampleRate(ec));
        if(ec)
            throw std::system_error(ec, "when reading sampling rate");

        WaveFile file;
        std::string res = file.open(filepath);
        if (!res.empty())
            throw std::runtime_error(res);

        if (!::hasSupportedSampleFormat(file))
            throw std::runtime_error("wave file doesn't have correct sample format");

        if (file.getChannels() != 2)
            throw std::runtime_error("wave file isn't a stereo file");

        if (file.getSamplerate() != samplerate)
            throw std::runtime_error("wave file hasn't got matching samplerate");

        bufferInfos[0].isInput = bufferInfos[1].isInput = false;
        bufferInfos[0].channelNum = firstChanIndex;
        bufferInfos[1].channelNum = firstChanIndex + 1;
        if(auto err = driver.createBuffers(bufferInfos.data(), bufferInfos.size(), preferredSize, &callbacks))
            throw std::system_error(err, cwASIO::err_category(), "when trying to create the buffers");
        blocksize = preferredSize;

        cwASIOSampleType sampleType = file.getBytesPerSample() == 4 ? ASIOSTInt32LSB : ASIOSTInt16LSB;
        for(long ch = 0; ch < long(std::size(channelInfos)); ++ch) {
            channelInfos[ch].channel = firstChanIndex + ch;
            channelInfos[ch].isInput = false;
            if(auto err = driver.getChannelInfo(channelInfos[ch]))
                throw std::system_error(err, cwASIO::err_category(), "when reading the info for channel with index " + std::to_string(ch));
            if(channelInfos[ch].type != sampleType)
                throw std::runtime_error("Sample type not supported on channel with index " + std::to_string(ch) + " (" + channelInfos[ch].name + ")");
        }

        uint64_t totalSamples = file.getTotalSamples();
        if(sampleType == ASIOSTInt32LSB) {
            fileBuffer32.resize(totalSamples * 2U);
            uint64_t sampleRead = file.read(totalSamples, fileBuffer32.data());
            if (sampleRead != totalSamples)
                throw std::runtime_error("couldn't read all samples of wave file");
        } else {
            fileBuffer16.resize(totalSamples * 2U);
            uint64_t sampleRead = file.read(totalSamples, fileBuffer16.data());
            if (sampleRead != totalSamples)
                throw std::runtime_error("couldn't read all samples of wave file");
        }
        file.close();

        uint64_t totalSeconds = totalSamples / samplerate;
        std::cout << "Now playing sound file for " << totalSeconds << " seconds\n";

        std::signal(SIGINT, signalHandler);

        if(auto err = driver.start())
            throw std::system_error(err, cwASIO::err_category(), "when trying to start streaming");

        std::cout << "Playback device " << driver.getDriverName()
            << " (" << channelInfos[0].name << "/" << channelInfos[1].name << ") at " << samplerate << " Hz\n";

        uint32_t limit = uint32_t(uint32_t(0) - 2 * blocksize * 8);     // file size limit 4GB
        uint32_t last = 0;
        while(signalStatus == 0 && stopStatus == 0)
            std::this_thread::sleep_for(10ms);
    } catch(std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}
