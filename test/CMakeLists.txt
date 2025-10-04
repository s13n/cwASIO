cmake_minimum_required(VERSION 3.20)

add_executable(cwASIO_application)

target_link_libraries(cwASIO_application PRIVATE cwASIO::lib)

target_sources(cwASIO_application PRIVATE
    application.c
)

add_executable(cwASIO_recorder)

target_link_libraries(cwASIO_recorder PRIVATE cwASIO::libxx cwASIO::lib)
target_compile_features(cwASIO_recorder PRIVATE cxx_std_20)

target_sources(cwASIO_recorder PRIVATE
    recorder.cpp
)

add_executable(cwASIO_player)

target_link_libraries(cwASIO_player PRIVATE cwASIO::libxx cwASIO::lib)
target_compile_features(cwASIO_player PRIVATE cxx_std_20)

target_sources(cwASIO_player PRIVATE
    player.cpp
    wavefile/wavefile.cpp
)
