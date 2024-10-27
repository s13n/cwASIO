/** @file       recorder.cpp
 *  @brief      cwASIO recording application
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2024
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO_test
 *  @{
 */

#include "cwASIO.hpp"
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::literals;

static std::mutex bufferMutex;
static std::queue<std::vector<int32_t>> bufferQueue;
static std::vector<cwASIOBufferInfo> bufferInfos(2);
static long blocksize{0};
static cwASIOChannelInfo channelInfos[2];
static std::sig_atomic_t volatile signalStatus = 0;

 
void signalHandler(int signal) {
    signalStatus = signal;
}

static std::vector<int32_t> getNext() {
    std::vector<int32_t> res;
    std::lock_guard<std::mutex> guard(bufferMutex);
    if(bufferQueue.empty())
        return res;
    res = std::move(bufferQueue.front());
    bufferQueue.pop();
    return res;
}

static void bufferSwitch(long doubleBufferIndex, cwASIOBool directProcess) {
    std::vector<int32_t> buf(2*blocksize);
    auto leftCh  = static_cast<int32_t const *>(bufferInfos[0].buffers[doubleBufferIndex]);
    auto rightCh = static_cast<int32_t const *>(bufferInfos[1].buffers[doubleBufferIndex]);
    for(long i = 0; i < blocksize; ++i) {
        buf[2*i]   = *leftCh++;
        buf[2*i+1] = *rightCh++;
    }
    std::lock_guard<std::mutex> guard(bufferMutex);
    bufferQueue.push(std::move(buf));
}

static void sampleRateDidChange(cwASIOSampleRate sRate) {
}

static long asioMessage(long selector, long value, void *message, double *opt) {
    return 0;
}

struct cwASIOTime *bufferSwitchTimeInfo(struct cwASIOTime *params, long doubleBufferIndex, cwASIOBool directProcess) {
    bufferSwitch(doubleBufferIndex, directProcess);
    return params;
}

static cwASIOCallbacks const callbacks = {
    .bufferSwitch = &bufferSwitch,
    .sampleRateDidChange = &sampleRateDidChange,
    .asioMessage = &asioMessage,
    .bufferSwitchTimeInfo = &bufferSwitchTimeInfo
};

static std::string getDriverId(std::string_view drivername) {
    std::string result(drivername);
    auto cb = [](void *res, char const *name, char const *id, char const *desc) -> bool {
        auto &result = *static_cast<std::string *>(res);
        if(result != name)
            return true;
        result = id;
        return false;
    };
    int err = cwASIOenumerate(cb, &result);
    if(err != 0)
        throw std::system_error(err, cwASIO::err_category(), "enumerating drivers");
    if(result == drivername)
        result.clear();
    return result;
}

struct WAVfile {
    struct Header {
        uint8_t fileTypeBlocID[4] = {0x52, 0x49, 0x46, 0x46};    // RIFF tag
        uint32_t fileSize = 36;                                  // Overall file size minus 8 bytes
        uint8_t fileFormatID[4] = {0x57, 0x41, 0x56, 0x45};      // WAVE tag
        uint8_t formatBlocID[4] = {0x66, 0x6D, 0x74, 0x20};      // fmt  tag
        uint32_t blocSize = 16;
        uint16_t audioFormat = 1;            // PCM integer
        uint16_t nbrChannels = 2;            // Number of channels
        uint32_t frequency;                  // Sample rate (in hertz)
        uint32_t bytePerSec;                 // Number of bytes to read per second (Frequency * BytePerBloc).
        uint16_t bytePerBloc = 8;            // Number of bytes per block (NbrChannels * BitsPerSample / 8).
        uint16_t bitsPerSample = 32;         // Number of bits per sample
        uint8_t dataBlocID[4] = {0x64, 0x61, 0x74, 0x61};        // data tag
        uint32_t dataSize = 0;               // SampledData size
    };

    WAVfile(std::filesystem::path path, uint32_t samplerate)
        : os_(path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary)
        , written_{sizeof(Header)}
    {
        Header header{ .frequency = samplerate, .bytePerSec = samplerate * 8 };
        os_.exceptions(std::ofstream::failbit);
        os_.write(reinterpret_cast<char const *>(&header), sizeof(header));
    }

    ~WAVfile() {
        // we need to fix up the header with the actual sizes
        std::streampos pos = offsetof(Header, fileSize);
        os_.seekp(pos);
        written_ -= pos;
        os_.write(reinterpret_cast<char const *>(&written_), sizeof(written_));
        written_ += pos;
        pos = offsetof(Header, dataSize);
        os_.seekp(pos);
        written_ -= pos;
        os_.write(reinterpret_cast<char const *>(&written_), sizeof(written_));
        os_.close();
        std::cout << "Written " << (written_ / 8) << " samples\n";
    }

    void write(std::vector<int32_t> const &vec) {
        os_.write(reinterpret_cast<char const *>(vec.data()), vec.size() * sizeof(int32_t));
        written_ += vec.size() * sizeof(int32_t);
    }

    std::ofstream os_;
    uint32_t written_;
};

/* Command line arguments are: */
int main(int argc, char const *argv[]) {
    if(argc != 4) {
        std::cout << "Usage: recorder <ASIO device> <first channel> <filename>\n";
        return 1;
    }

    try {
        std::error_code ec;
        cwASIO::Driver driver(getDriverId(argv[1]));
        auto channel = strtoul(argv[2], nullptr, 10);
        std::filesystem::path filepath(argv[3]);

        if(!driver.init(nullptr))
            throw std::runtime_error("Can't init driver " + driver.getDriverName() + " version "
                    + std::to_string(driver.getDriverVersion()) + ": " + driver.getErrorMessage());

        auto [numInputChannels, _] = driver.getChannels(ec);
        if(ec)
            throw std::system_error(ec, "when reading number of channels");
        if(numInputChannels <= channel+1)
            throw std::runtime_error("requested input channels not available");

        auto [_0, _1, preferredSize, _2] = driver.getBufferSize(ec);
        if(ec)
            throw std::system_error(ec, "when reading supported buffer sizes");

        bufferInfos[0].isInput = bufferInfos[1].isInput = true;
        bufferInfos[0].channelNum = channel;
        bufferInfos[1].channelNum = channel + 1;
        if(auto err = driver.createBuffers(bufferInfos.data(), bufferInfos.size(), preferredSize, &callbacks))
            throw std::system_error(err, cwASIO::err_category(), "when trying to create the buffers");
        blocksize = preferredSize;

        for(size_t ch = 0; ch < std::size(channelInfos); ++ch) {
            channelInfos[ch].channel = channel + ch;
            channelInfos[ch].isInput = true;
            if(auto err = driver.getChannelInfo(channelInfos[ch]))
                throw std::system_error(err, cwASIO::err_category(), "when reading the info for channel " + std::to_string(ch));
            if(channelInfos[ch].type != ASIOSTInt32LSB)
                throw std::runtime_error("Sample type not supported on channel " + std::to_string(ch) + " (" + channelInfos[ch].name + ")");
        }

        uint32_t samplerate = uint32_t(driver.getSampleRate(ec));
        if(ec)
            throw std::system_error(ec, "when reading sampling rate");

        WAVfile file(filepath, samplerate);
        
        std::signal(SIGINT, signalHandler);
        
        if(auto err = driver.start())
            throw std::system_error(err, cwASIO::err_category(), "when trying to start streaming");

        std::cout << "Recording device " << driver.getDriverName() << " channels " << channel << "&" << (channel + 1) << " at " << samplerate << " Hz\n";

        while(signalStatus == 0) {
            std::vector<int32_t> buf = getNext();
            if(buf.empty())
                std::this_thread::sleep_for(10ms);
            else
                file.write(buf);
        }
    } catch(std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
}
