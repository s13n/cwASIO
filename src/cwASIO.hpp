/** @file       cwASIO.hpp
 *  @brief      cwASIO C++ wrapper for hosts
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */
#pragma once

extern "C" {
    #include "cwASIO.h"
}
#include <chrono>
#include <memory>
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

    /** Driver handle. */
    struct Driver {
    private:
        std::unique_ptr<cwASIODriver, void(*)(cwASIODriver*)> drv_;

        [[noreturn]] void throwError();

    public:
        Driver() = default;
        explicit Driver(std::string id, std::string name);

        // The following wrapper functions are supposed to ease debugging. They should get optimized away in a release build.

        bool init(void *sysHandle) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->init(drv_.get(), sysHandle);
        }

        std::string getDriverName();

        long getDriverVersion() {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->getDriverVersion(drv_.get());
        }

        std::string getErrorMessage();

        cwASIOError start() {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->start(drv_.get());
        }

        cwASIOError stop() {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->stop(drv_.get());
        }

        std::tuple<long, long> getChannels(std::error_code &ec) {
            if(!drv_)
                throwError();
            long numInputChannels, numOutputChannels;
            if(auto err = drv_->lpVtbl->getChannels(drv_.get(), &numInputChannels, &numOutputChannels))
                ec.assign(err, err_category());
            return { numInputChannels, numOutputChannels };
        }

        std::tuple<long, long> getLatencies(std::error_code &ec) {
            if(!drv_)
                throwError();
            long inputLatency, outputLatency;
            if(auto err = drv_->lpVtbl->getLatencies(drv_.get(), &inputLatency, &outputLatency))
                ec.assign(err, err_category());
            return { inputLatency, outputLatency };
        }

        std::tuple<long, long, long, long> getBufferSize(std::error_code &ec) {
            if(!drv_)
                throwError();
            long minSize, maxSize, preferredSize, granularity;
            if(auto err = drv_->lpVtbl->getBufferSize(drv_.get(), &minSize, &maxSize, &preferredSize, &granularity))
                ec.assign(err, err_category());
            return { minSize, maxSize, preferredSize, granularity };
        }

        cwASIOError canSampleRate(double sampleRate) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->canSampleRate(drv_.get(), sampleRate);
        }

        double getSampleRate(std::error_code &ec) {
            if(!drv_)
                throwError();
            double sampleRate;
            if(auto err = drv_->lpVtbl->getSampleRate(drv_.get(), &sampleRate))
                ec.assign(err, err_category());
            return sampleRate;
        }

        cwASIOError setSampleRate(double sampleRate) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->setSampleRate(drv_.get(), sampleRate);
        }

        std::vector<cwASIOClockSource> getClockSources(std::error_code &ec);

        cwASIOError setClockSource(long reference) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->setClockSource(drv_.get(), reference);
        }

        SamplePosition getSamplePosition(std::error_code &ec) {
            if(!drv_)
                throwError();
            cwASIOSamples asp;
            cwASIOTimeStamp ats;
            if(auto err = drv_->lpVtbl->getSamplePosition(drv_.get(), &asp, &ats))
                ec.assign(err, err_category());
            return SamplePosition{ std::chrono::nanoseconds(qWord(ats)), qWord(asp) };
        }

        cwASIOError getChannelInfo(cwASIOChannelInfo &info) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->getChannelInfo(drv_.get(), &info);
        }

        cwASIOError createBuffers(cwASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, cwASIOCallbacks const *callbacks) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->createBuffers(drv_.get(), bufferInfos, numChannels, bufferSize, callbacks);
        }

        cwASIOError disposeBuffers() {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->disposeBuffers(drv_.get());
        }

        cwASIOError controlPanel() {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->controlPanel(drv_.get());
        }

        cwASIOError future(long selector, void *opt) {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->future(drv_.get(), selector, opt);
        }

        cwASIOError outputReady() {
            if(!drv_)
                throwError();
            return drv_->lpVtbl->outputReady(drv_.get());
        }
    };

} // namespace
