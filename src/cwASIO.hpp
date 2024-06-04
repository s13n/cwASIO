/** @file       cwASIO.hpp
 *  @brief      cwASIO C++ wrapper for hosts
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
#include <chrono>
#include <expected>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>


namespace cwASIO {

    /** Error category for cwASIO errors, for use with `std::system_error`.
     * This error category allows printing of error text for errors originating in the driver.
     */
    class Errc_category : public std::error_category {
    public:
        const char *name() const noexcept override final {
            return "cwASIO Error";
        }

        std::string message(int c) const override final {
            switch (c) {
            case ASE_OK:               return "success";
            case ASE_SUCCESS:          return "successful future() call";
            case ASE_NotPresent:       return "hardware input or output is not present or available";
            case ASE_HWMalfunction:    return "hardware is malfunctioning";
            case ASE_InvalidParameter: return "input parameter invalid";
            case ASE_InvalidMode:      return "hardware is in a bad mode or used in a bad mode";
            case ASE_SPNotAdvancing:   return "hardware is not running when sample position is inquired";
            case ASE_NoClock:          return "sample clock or rate cannot be determined or is not present";
            case ASE_NoMemory:         return "not enough memory for completing the request";
            default:                   return "general error";
            }
        }
    };

    static Errc_category const &err_category() {
        static Errc_category cat;
        return cat;
    }

    template<typename T> inline uint64_t qWord(T const &val) {
        return val.hi * 0x100000000ULL + val.lo;
    };

    template<> inline uint64_t qWord<long long int>(long long int const &val) {
        return uint64_t(val);
    };

    struct SamplePosition {
        std::chrono::nanoseconds systemTime;
        uint64_t samplePosition;
    };

    struct Driver {
    private:
        struct cwASIODriver *drv_;

        Driver(Driver &&) = delete;

    public:
        explicit Driver(std::string id)
            : drv_{ nullptr }
        {
            auto err = cwASIOload(id.c_str(), &drv_);
            if (err || !drv_ || !drv_->lpVtbl)
                throw std::runtime_error("can't load cwASIO driver " + id + " err=" + std::to_string(err));
        }

        ~Driver() {
            cwASIOunload(drv_);
        }

        // The following wrapper functions are supposed to ease debugging. They should get optimized away in a release build.

        bool init(void *sysHandle) {
            return drv_->lpVtbl->init(drv_, sysHandle);
        }

        void getDriverName(char *name) {
            drv_->lpVtbl->getDriverName(drv_, name);
        }

        long getDriverVersion() {
            return drv_->lpVtbl->getDriverVersion(drv_);
        }

        std::string getErrorMessage() {
            std::string errorMessage(124, '\0');
            drv_->lpVtbl->getErrorMessage(drv_, errorMessage.data());
            errorMessage.resize(std::min(errorMessage.find('\0'), errorMessage.length()));
            return errorMessage;
        }

        cwASIOError start() {
            return drv_->lpVtbl->start(drv_);
        }

        cwASIOError stop() {
            return drv_->lpVtbl->stop(drv_);
        }

        std::tuple<cwASIOError, long, long> getChannels() {
            long numInputChannels, numOutputChannels;
            auto err = drv_->lpVtbl->getChannels(drv_, &numInputChannels, &numOutputChannels);
            return { err, numInputChannels, numOutputChannels };
        }

        std::tuple<cwASIOError, long, long> getLatencies() {
            long inputLatency, outputLatency;
            auto err = drv_->lpVtbl->getLatencies(drv_, &inputLatency, &outputLatency);
            return { err, inputLatency, outputLatency };
        }

        std::tuple<cwASIOError, long, long, long, long> getBufferSize() {
            long minSize, maxSize, preferredSize, granularity;
            auto err = drv_->lpVtbl->getBufferSize(drv_, &minSize, &maxSize, &preferredSize, &granularity);
            return { err, minSize, maxSize, preferredSize, granularity };
        }

        cwASIOError canSampleRate(double sampleRate) {
            return drv_->lpVtbl->canSampleRate(drv_, sampleRate);
        }

        std::expected<double, std::error_code> getSampleRate() {
            double sampleRate;
            auto err = drv_->lpVtbl->getSampleRate(drv_, &sampleRate);
            if (err)
                return std::unexpected(std::error_code(err, err_category()));
            return sampleRate;
        }

        cwASIOError setSampleRate(double sampleRate) {
            return drv_->lpVtbl->setSampleRate(drv_, sampleRate);
        }

        std::expected<std::vector<cwASIOClockSource>, std::error_code> getClockSources() {
            std::vector<cwASIOClockSource> clocks(1);
            long numSources = long(clocks.size());
            auto err = drv_->lpVtbl->getClockSources(drv_, clocks.data(), &numSources);
            if (size_t(numSources) > clocks.size()) {
                clocks.resize(numSources);
                err = drv_->lpVtbl->getClockSources(drv_, clocks.data(), &numSources);
            }
            if (err)
                return std::unexpected(std::error_code(err, err_category()));
            return clocks;
        }

        cwASIOError setClockSource(long reference) {
            return drv_->lpVtbl->setClockSource(drv_, reference);
        }

        std::expected<SamplePosition, std::error_code> getSamplePosition() {
            cwASIOSamples asp;
            cwASIOTimeStamp ats;
            if(auto err = drv_->lpVtbl->getSamplePosition(drv_, &asp, &ats))
                return std::unexpected(std::error_code(err, err_category()));
            return SamplePosition{ std::chrono::nanoseconds(qWord(ats)), qWord(asp) };
        }

        cwASIOError getChannelInfo(cwASIOChannelInfo &info) {
            return drv_->lpVtbl->getChannelInfo(drv_, &info);
        }

        cwASIOError createBuffers(cwASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, cwASIOCallbacks const *callbacks) {
            return drv_->lpVtbl->createBuffers(drv_, bufferInfos, numChannels, bufferSize, callbacks);
        }

        cwASIOError disposeBuffers() {
            return drv_->lpVtbl->disposeBuffers(drv_);
        }

        cwASIOError controlPanel() {
            return drv_->lpVtbl->controlPanel(drv_);
        }

        cwASIOError future(long selector, void *opt) {
            return drv_->lpVtbl->future(drv_, selector, opt);
        }

        cwASIOError outputReady() {
            return drv_->lpVtbl->outputReady(drv_);
        }
    };

} // namespace
