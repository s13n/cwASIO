cmake_minimum_required(VERSION 3.20)

add_library(cwASIO_lib OBJECT)
add_library(cwASIO::lib ALIAS cwASIO_lib)

target_include_directories(cwASIO_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(cwASIO_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_property(TARGET cwASIO_lib PROPERTY
    PUBLIC_HEADER cwASIOtypes.h cwASIO.h
)

target_sources(cwASIO_lib
PUBLIC
    cwASIO.h
    cwASIOtypes.h
PRIVATE
    cwASIO.c
)

# Build the driver library
add_library(cwASIO_driver OBJECT cwASIO.c cwASIOdriver.c)
add_library(cwASIO::driver ALIAS cwASIO_driver)
set_target_properties(cwASIO_driver PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_property(TARGET cwASIO_driver PROPERTY
    PUBLIC_HEADER cwASIOtypes.h cwASIO.h cwASIOdriver.h cwASIOdriver.def
)
target_sources(cwASIO_driver PUBLIC cwASIOtypes.h cwASIO.h cwASIOdriver.h)
target_include_directories(cwASIO_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Define C++ wrapper as an object library
add_library(cwASIO_libxx OBJECT cwASIO.hpp cwASIO.cpp)
add_library(cwASIO::libxx ALIAS cwASIO_libxx)
target_compile_features(cwASIO_libxx PUBLIC cxx_std_17)
target_link_libraries(cwASIO_libxx PUBLIC cwASIO::lib)
set_target_properties(cwASIO_libxx PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_property(TARGET cwASIO_libxx PROPERTY PUBLIC_HEADER cwASIO.hpp)

# Build ASIO compatibility wrapper
add_library(cwASIO_asio OBJECT asio/asio.c asio/asio.h)
add_library(cwASIO::asio ALIAS cwASIO_asio)
target_link_libraries(cwASIO_asio PUBLIC cwASIO::lib)
target_include_directories(cwASIO_asio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/asio)

include(GNUInstallDirs)

install(TARGETS cwASIO_lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cwASIO
)
