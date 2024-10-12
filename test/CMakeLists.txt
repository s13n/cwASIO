cmake_minimum_required(VERSION 3.20)

add_executable(cwASIO_application)

target_link_libraries(cwASIO_application PRIVATE cwASIO::lib)

target_sources(cwASIO_application PRIVATE
    application.c
)
