
add_library(cwASIO_lib)

target_sources(cwASIO_lib
PUBLIC
    cwASIO.h
    $<IF:${WIN32},cwASIOifc.hpp,cwASIOifc.h>
PRIVATE
    $<IF:${WIN32},cwASIO.cpp,cwASIO.c>
)

target_include_directories(cwASIO_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(cwASIO_lib PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON           # Needed when building ptp++client as a DLL
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<BOOL:${PTP_RUNTIME_SHARED}>:DLL>"
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/src/cwASIO.h;${PROJECT_SOURCE_DIR}/src/cwASIOifc.h;${PROJECT_SOURCE_DIR}/src/cwASIO.hpp"
)

add_library(cwASIO::lib ALIAS cwASIO_lib)
