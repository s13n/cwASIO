/** @file       cwASIO.cpp
 *  @brief      cwASIO C++ wrapper for hosts
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO
 *  @{
 */

#include "cwASIO.hpp"


const char *cwASIO::Errc_category::name() const noexcept {
    return "cwASIO Error";
}

std::string cwASIO::Errc_category::message(int c) const {
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


void cwASIO::Driver::throwError() {
    throw std::system_error(ASE_NotPresent, err_category(), "no driver loaded");
}

cwASIO::Driver::Driver(std::string id, std::string name)
    : Driver{}
{
    cwASIODriver *drv;
    auto err = cwASIOload(id.c_str(), &drv);
    if (err || !drv || !drv->lpVtbl) {
#ifdef _WIN32
        std::error_code ec(err, std::system_category());
#else
        std::error_code ec(err, err_category());
#endif
        throw std::system_error(ec, "can't load cwASIO driver " + name + " (" + id + ")");
    }

    drv_.reset(drv);

    err = future(kcwASIOsetInstanceName, const_cast<char*>(name.c_str()));
    switch (err) {
    case ASE_SUCCESS:           // driver offers multiinstance support
        return;
    case ASE_InvalidParameter:  // driver offers no multiinstance support, that's OK
        return;
    case ASE_NotPresent:        // driver didn't find its registry for the given instance, not OK!
        throw std::system_error(err, err_category(), "driver " + name + " not registered");
    default:
        throw std::system_error(err, err_category(), "setting instance name on driver: " + name);
    }
}

std::string cwASIO::Driver::getDriverName() {
    if(!drv_)
        throwError();
    std::string name(32, '\0');
    drv_->lpVtbl->getDriverName(drv_.get(), name.data());
    name.resize(std::min(name.find('\0'), name.length()));
    return name;
}

std::string cwASIO::Driver::getErrorMessage() {
    if(!drv_)
        throwError();
    std::string errorMessage(124, '\0');
    drv_->lpVtbl->getErrorMessage(drv_.get(), errorMessage.data());
    errorMessage.resize(std::min(errorMessage.find('\0'), errorMessage.length()));
    return errorMessage;
}

std::vector<cwASIOClockSource> cwASIO::Driver::getClockSources(std::error_code &ec) {
    if(!drv_)
        throwError();
    std::vector<cwASIOClockSource> clocks(1);
    long numSources = long(clocks.size());
    auto err = drv_->lpVtbl->getClockSources(drv_.get(), clocks.data(), &numSources);
    if (size_t(numSources) > clocks.size()) {
        clocks.resize(numSources);
        err = drv_->lpVtbl->getClockSources(drv_.get(), clocks.data(), &numSources);
    }
    if (err)
        ec.assign(err, err_category());
    return clocks;
}
