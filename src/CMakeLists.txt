
add_library(cwASIO)

target_sources(cwASIO PUBLIC
    cwASIO.h
    $<IF:${WIN32},cwASIOifc.hpp,cwASIOifc.h>
    $<IF:${WIN32},cwASIO.cpp,cwASIO.c>
)
