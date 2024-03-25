
add_library(cwASIO_lib)

target_sources(cwASIO_lib
PUBLIC
    cwASIO.h
    $<IF:${WIN32},cwASIOifc.hpp,cwASIOifc.h>
PRIVATE
    $<IF:${WIN32},cwASIO.cpp,cwASIO.c>
)

add_library(cwASIO::lib ALIAS cwASIO_lib)
