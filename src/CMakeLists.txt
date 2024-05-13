cmake_minimum_required(VERSION 3.20)

option(CWASIO_SHARED "Build cwASIO as a shared library" ${BUILD_SHARED_LIBS})
if(CWASIO_SHARED)
    set(CWASIO_LIBTYPE SHARED)
else()
    set(CWASIO_LIBTYPE STATIC)
endif()

add_library(cwASIO_lib ${CWASIO_LIBTYPE})
message(VERBOSE "Building cwASIO as a ${CWASIO_LIBTYPE} library")
add_library(cwASIO::lib ALIAS cwASIO_lib)

if(WIN32)
    target_sources(cwASIO_lib
    PUBLIC
        cwASIO.h
        cwASIOifc.hpp
    PRIVATE
        cwASIO.cpp
    )
    
    set_target_properties(cwASIO_lib PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        POSITION_INDEPENDENT_CODE ON
        PUBLIC_HEADER "cwASIO.h;cwASIOifc.hpp"
    )

    if(NOT DEFINED CMAKE_MODULE_PATH OR CMAKE_MODULE_PATH STREQUAL "")
        if(NOT DEFINED CMAKE_MODULE_PATH)
            set(CWASIO_CMAKE_MODULE_PATH_SET 1)
        endif()
        set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    else()
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    find_package(ATL)
    if(DEFINED CWASIO_CMAKE_MODULE_PATH_SET)
        unset(CMAKE_MODULE_PATH)
        unset(CWASIO_CMAKE_MODULE_PATH_SET)
    endif()
    if(NOT ATL_FOUND)
        message(FATAL_ERROR "ATL was not found, please install Desktop development with C++ / ATL for newest build tools!")
    endif()
else()
    target_sources(cwASIO_lib
    PUBLIC
        cwASIO.h
        cwASIOifc.h
    PRIVATE
        cwASIO.c
    )
    
    set_target_properties(cwASIO_lib PROPERTIES PUBLIC_HEADER "cwASIO.h;cwASIOifc.h")
endif()

target_include_directories(cwASIO_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include(GNUInstallDirs)

install(TARGETS cwASIO_lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cwASIO
)
