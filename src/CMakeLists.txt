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
    set_target_properties(cwASIO_lib PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        POSITION_INDEPENDENT_CODE ON
    )
endif()

target_sources(cwASIO_lib
PUBLIC
    cwASIO.h
    cwASIOtypes.h
PRIVATE
    cwASIO.c
)

set_property(TARGET cwASIO_lib PROPERTY PUBLIC_HEADER asio.h cwASIOtypes.h cwASIO.h)
target_include_directories(cwASIO_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Build the driver library
add_library(cwASIO_driver STATIC)
add_library(cwASIO::driver ALIAS cwASIO_driver)
target_sources(cwASIO_driver PRIVATE cwASIOtypes.h cwASIO.h cwASIO.c cwASIOdriver.h cwASIOdriver.c cwASIOdriver.def)
target_include_directories(cwASIO_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cwASIO_driver PUBLIC c_std_11)

# Define C++ wrapper as an interface library
add_library(cwASIO_libxx INTERFACE cwASIO.hpp)
target_compile_features(cwASIO_libxx INTERFACE cxx_std_23)
target_link_libraries(cwASIO_libxx INTERFACE cwASIO::lib)
set_property(TARGET cwASIO_libxx PROPERTY PUBLIC_HEADER cwASIO.hpp)

# Build ASIO compatibility wrapper
add_library(cwASIO_asio ${CWASIO_LIBTYPE} asio/asio.c asio/asio.h)
add_library(cwASIO::asio ALIAS cwASIO_asio)
target_link_libraries(cwASIO_asio PRIVATE cwASIO::lib)
target_include_directories(cwASIO_asio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/asio)

include(GNUInstallDirs)

install(TARGETS cwASIO_lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cwASIO
)
