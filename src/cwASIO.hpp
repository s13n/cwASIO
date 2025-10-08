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
        const char *name() const noexcept override final;
        std::string message(int) const override final;
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
        explicit Driver(std::string id, std::string name);

        ~Driver() {
            cwASIOunload(drv_);
        }

        // The following wrapper functions are supposed to ease debugging. They should get optimized away in a release build.

        bool init(void *sysHandle) {
            return drv_->lpVtbl->init(drv_, sysHandle);
        }

        std::string getDriverName();

        long getDriverVersion() {
            return drv_->lpVtbl->getDriverVersion(drv_);
        }

        std::string getErrorMessage();

        cwASIOError start() {
            return drv_->lpVtbl->start(drv_);
        }

        cwASIOError stop() {
            return drv_->lpVtbl->stop(drv_);
        }

        std::tuple<long, long> getChannels(std::error_code &ec) {
            long numInputChannels, numOutputChannels;
            if(auto err = drv_->lpVtbl->getChannels(drv_, &numInputChannels, &numOutputChannels))
                ec.assign(err, err_category());
            return { numInputChannels, numOutputChannels };
        }

        std::tuple<long, long> getLatencies(std::error_code &ec) {
            long inputLatency, outputLatency;
            if(auto err = drv_->lpVtbl->getLatencies(drv_, &inputLatency, &outputLatency))
                ec.assign(err, err_category());
            return { inputLatency, outputLatency };
        }

        std::tuple<long, long, long, long> getBufferSize(std::error_code &ec) {
            long minSize, maxSize, preferredSize, granularity;
            if(auto err = drv_->lpVtbl->getBufferSize(drv_, &minSize, &maxSize, &preferredSize, &granularity))
                ec.assign(err, err_category());
            return { minSize, maxSize, preferredSize, granularity };
        }

        cwASIOError canSampleRate(double sampleRate) {
            return drv_->lpVtbl->canSampleRate(drv_, sampleRate);
        }

        double getSampleRate(std::error_code &ec) {
            double sampleRate;
            if(auto err = drv_->lpVtbl->getSampleRate(drv_, &sampleRate))
                ec.assign(err, err_category());
            return sampleRate;
        }

        cwASIOError setSampleRate(double sampleRate) {
            return drv_->lpVtbl->setSampleRate(drv_, sampleRate);
        }

        std::vector<cwASIOClockSource> getClockSources(std::error_code &ec);

        cwASIOError setClockSource(long reference) {
            return drv_->lpVtbl->setClockSource(drv_, reference);
        }

        SamplePosition getSamplePosition(std::error_code &ec) {
            cwASIOSamples asp;
            cwASIOTimeStamp ats;
            if(auto err = drv_->lpVtbl->getSamplePosition(drv_, &asp, &ats))
                ec.assign(err, err_category());
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
