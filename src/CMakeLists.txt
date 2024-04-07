cmake_minimum_required(VERSION 3.20)

option(CWASIO_SHARED "Build cwASIO as a shared library" ${BUILD_SHARED_LIBS})
if(CWASIO_SHARED)
    add_library(cwASIO_lib SHARED)
    message(VERBOSE "Building cwASIO as a shared library")
else()
    add_library(cwASIO_lib STATIC)
    message(VERBOSE "Building cwASIO as a static library")
endif()
add_library(cwASIO::lib ALIAS cwASIO_lib)

if(WIN32)
    target_sources(cwASIO_lib
    PUBLIC
        cwASIO.h
        cwASIOifc.hpp
    PRIVATE
        cwASIO.cpp
    )

    option(CWASIO_SHARED_RUNTIME "Build cwASIO using the shared runtime library" ${CWASIO_SHARED})
    
    set_target_properties(cwASIO_lib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<BOOL:${CWASIO_SHARED_RUNTIME}>:DLL>"
        PUBLIC_HEADER "cwASIO.h;cwASIOifc.hpp"
    )

    find_path(ATL_HEADER atlbase.h DOC "ATL atlbase.h header file")
    message(VERBOSE "Using atlbase.h in folder ${ATL_HEADER}")
    find_path(UNK_HEADER unknwn.h DOC "COM unknwn.h header file")
    message(VERBOSE "Using unknwn.h in folder ${UNK_HEADER}")
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

set_target_properties(cwASIO_lib PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<BOOL:${PTP_RUNTIME_SHARED}>:DLL>"
)

include(GNUInstallDirs)

install(TARGETS cwASIO_lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cwASIO
)
