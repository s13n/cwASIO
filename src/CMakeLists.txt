
add_library(cwASIO_lib)

if(WIN32)
    target_sources(cwASIO_lib
    PUBLIC FILE_SET HEADERS FILES
        cwASIO.h
        cwASIOifc.hpp
    PRIVATE
        cwASIO.cpp
    )
    find_path(ATL_HEADER atlbase.h DOC "ATL atlbase.h header file")
    message(VERBOSE "Using atlbase.h in folder ${ATL_HEADER}")
    find_path(UNK_HEADER unknwn.h DOC "COM unknwn.h header file")
    message(VERBOSE "Using unknwn.h in folder ${UNK_HEADER}")
else()
    target_sources(cwASIO_lib
    PUBLIC FILE_SET HEADERS FILES
        cwASIO.h
        cwASIOifc.h
    PRIVATE
        cwASIO.c
    )
endif()

target_include_directories(cwASIO_lib PUBLIC .)

set_target_properties(cwASIO_lib PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<BOOL:${PTP_RUNTIME_SHARED}>:DLL>"
)

add_library(cwASIO::lib ALIAS cwASIO_lib)
