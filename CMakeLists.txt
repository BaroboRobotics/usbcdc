cmake_minimum_required(VERSION 3.2.1)
project(usbcdc VERSION 0.1.0)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.54.0 REQUIRED COMPONENTS system filesystem iostreams)
find_package(cxx-util)

set(SOURCES src/deviceoperators.cpp)
if(WIN32)
    list(APPEND SOURCES src/windows/devices.cpp)
elseif(APPLE)
    list(APPEND SOURCES src/macos/devices.cpp)
else()
    list(APPEND SOURCES src/linux/devices.cpp src/linux/parseudevadm.cpp)
endif()

add_library(usbcdc STATIC ${SOURCES})

set_target_properties(usbcdc
    PROPERTIES CXX_STANDARD 14
               CXX_STANDARD_REQUIRED ON
               )

target_include_directories(usbcdc
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
target_link_libraries(usbcdc
    PUBLIC
        barobo::cxx-util Boost::system Boost::filesystem Boost::iostreams
)

if(WIN32)
    target_link_libraries(usbcdc PRIVATE setupapi)
elseif(APPLE)
    add_subdirectory(CFPP)
    find_library(coreFoundationLib CoreFoundation)
    find_library(ioKitLib IOKit)
    target_link_libraries(usbcdc
      PRIVATE cfpp
              ${ioKitLib}
              ${coreFoundationLib}
              )
endif()

set_target_properties(usbcdc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    MACOSX_RPATH ON
)

install(TARGETS usbcdc EXPORT barobo
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

# Boilerplate follows

export(EXPORT barobo
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake"
    NAMESPACE barobo::
)

install(EXPORT barobo
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE barobo::
    DESTINATION cmake
)

configure_file(${PROJECT_NAME}-config.cmake.in ${PROJECT_NAME}-config.cmake @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-configVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-configVersion.cmake"
    DESTINATION cmake
)
