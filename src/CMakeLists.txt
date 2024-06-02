cmake_minimum_required(VERSION 3.20)

include(FetchContent)

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
        message(STATUS "ATL was not found: using WTL4MinGW!")
        set(CWASIO_USE_WTL4MINGW_REPO "https://github.com/aholzinger/WTL4MinGW.git" CACHE STRING "wtl4mingw repository to use")
        if(NOT CWASIO_USE_WTL4MINGW_TAG)
            if($ENV{CWASIO_USE_WTL4MINGW_TAG})
                set(CWASIO_USE_WTL4MINGW_TAG $ENV{CWASIO_USE_WTL4MINGW_TAG})
            else()
                set(CWASIO_USE_WTL4MINGW_TAG "8c3ea308c11fd2c6bae03412b7ef98ec94c7b0c2")   # set default tag, when neither cache nor environment variable found.
            endif()
        endif()

        FetchContent_Declare(WTL4MinGW_build
            GIT_TAG         ${CWASIO_USE_WTL4MINGW_TAG}
            GIT_REPOSITORY  ${CWASIO_USE_WTL4MINGW_REPO}
        )
        message(STATUS "Fetching wtl4mingw sources ${CWASIO_USE_WTL4MINGW_TAG} from git at ${CWASIO_USE_WTL4MINGW_REPO}")
        FetchContent_GetProperties(wtl4mingw_build)
        if(NOT wtl4mingw_build_POPULATED)
            FetchContent_Populate(wtl4mingw_build)
            message(STATUS "Building wtl4mingw in ${wtl4mingw_build_BINARY_DIR} from sources in ${wtl4mingw_build_SOURCE_DIR}")
            add_subdirectory(${wtl4mingw_build_SOURCE_DIR} ${wtl4mingw_build_BINARY_DIR} EXCLUDE_FROM_ALL)
        endif()
        target_compile_definitions(cwASIO_lib PRIVATE CWASIO_USE_WTL4MINGW=1)
        set(PKGCONFIG_CFLAGS "${PKGCONFIG_CFLAGS} -DCWASIO_USE_WTL4MINGW=1")
        target_include_directories(cwASIO_lib PRIVATE ${wtl4mingw_build_SOURCE_DIR}/src)
    endif()
    
    set_target_properties(cwASIO_lib PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        POSITION_INDEPENDENT_CODE ON
    )
else()
endif()

target_sources(cwASIO_lib
PUBLIC
    asio.h
    cwASIO.h
    cwASIOifc.h
    cwASIOifc.hpp
PRIVATE
    asio.c
    cwASIO.c
)

set_target_properties(cwASIO_lib PROPERTIES PUBLIC_HEADER "asio.h;cwASIO.h;cwASIOifc.h;cwASIOifc.hpp")
target_compile_features(cwASIO_lib PRIVATE cxx_std_23)
target_include_directories(cwASIO_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include(GNUInstallDirs)

install(TARGETS cwASIO_lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cwASIO
)
