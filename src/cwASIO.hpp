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
#include <cassert>
#include <chrono>
#include <cstring>
#include <memory>
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

    /** Handle for an ASIO device. */
    struct Device {
    private:
        std::unique_ptr<cwASIODriver, void(*)(cwASIODriver*)> drv_;

    public:
        Device() : drv_{ nullptr, &cwASIOunload } {}
        explicit Device(std::string name);

        /** Initialize the driver instance for the device.
         * @param sysHandle The system handle (see ASIO documentation)
         * @return Collected driver info.
         *
         * The returned struct contains an error text buffer that contains an
         * empty string on success, and an error text on failure.
         */
        cwASIODriverInfo init(void *sysHandle);

        std::string getDriverName();

        long getDriverVersion() {
            assert(drv_);
            return drv_->lpVtbl->getDriverVersion(drv_.get());
        }

        std::string getErrorMessage();

        cwASIOError start() {
            assert(drv_);
            return drv_->lpVtbl->start(drv_.get());
        }

        cwASIOError stop() {
            assert(drv_);
            return drv_->lpVtbl->stop(drv_.get());
        }

        std::tuple<long, long> getChannels(std::error_code &ec) {
            assert(drv_);
            long numInputChannels, numOutputChannels;
            if(auto err = drv_->lpVtbl->getChannels(drv_.get(), &numInputChannels, &numOutputChannels))
                ec.assign(err, err_category());
            return { numInputChannels, numOutputChannels };
        }

        std::tuple<long, long> getLatencies(std::error_code &ec) {
            assert(drv_);
            long inputLatency, outputLatency;
            if(auto err = drv_->lpVtbl->getLatencies(drv_.get(), &inputLatency, &outputLatency))
                ec.assign(err, err_category());
            return { inputLatency, outputLatency };
        }

        std::tuple<long, long, long, long> getBufferSize(std::error_code &ec) {
            assert(drv_);
            long minSize, maxSize, preferredSize, granularity;
            if(auto err = drv_->lpVtbl->getBufferSize(drv_.get(), &minSize, &maxSize, &preferredSize, &granularity))
                ec.assign(err, err_category());
            return { minSize, maxSize, preferredSize, granularity };
        }

        cwASIOError canSampleRate(double sampleRate) {
            assert(drv_);
            return drv_->lpVtbl->canSampleRate(drv_.get(), sampleRate);
        }

        double getSampleRate(std::error_code &ec) {
            assert(drv_);
            double sampleRate;
            if(auto err = drv_->lpVtbl->getSampleRate(drv_.get(), &sampleRate))
                ec.assign(err, err_category());
            return sampleRate;
        }

        cwASIOError setSampleRate(double sampleRate) {
            assert(drv_);
            return drv_->lpVtbl->setSampleRate(drv_.get(), sampleRate);
        }

        std::vector<cwASIOClockSource> getClockSources(std::error_code &ec);

        cwASIOError setClockSource(long reference) {
            assert(drv_);
            return drv_->lpVtbl->setClockSource(drv_.get(), reference);
        }

        SamplePosition getSamplePosition(std::error_code &ec) {
            assert(drv_);
            cwASIOSamples asp;
            cwASIOTimeStamp ats;
            if(auto err = drv_->lpVtbl->getSamplePosition(drv_.get(), &asp, &ats))
                ec.assign(err, err_category());
            return SamplePosition{ std::chrono::nanoseconds(qWord(ats)), qWord(asp) };
        }

        cwASIOError getChannelInfo(cwASIOChannelInfo &info) {
            assert(drv_);
            return drv_->lpVtbl->getChannelInfo(drv_.get(), &info);
        }

        cwASIOError createBuffers(cwASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, cwASIOCallbacks const *callbacks) {
            assert(drv_);
            return drv_->lpVtbl->createBuffers(drv_.get(), bufferInfos, numChannels, bufferSize, callbacks);
        }

        cwASIOError disposeBuffers() {
            assert(drv_);
            return drv_->lpVtbl->disposeBuffers(drv_.get());
        }

        cwASIOError controlPanel() {
            assert(drv_);
            return drv_->lpVtbl->controlPanel(drv_.get());
        }

        cwASIOError future(long selector, void *opt) {
            assert(drv_);
            return drv_->lpVtbl->future(drv_.get(), selector, opt);
        }

        cwASIOError outputReady() {
            assert(drv_);
            return drv_->lpVtbl->outputReady(drv_.get());
        }
    };

} // namespace
