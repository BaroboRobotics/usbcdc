cmake_minimum_required(VERSION 3.2.1)
project(usbcdc VERSION 0.1.0)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.59.0 REQUIRED COMPONENTS system filesystem)
find_package(cxx-util)

if(APPLE)
    add_subdirectory(CFPP)
endif()

set(SOURCES src/devices.cpp)

add_library(usbcdc STATIC ${SOURCES})

set_target_properties(usbcdc
    PROPERTIES CXX_STANDARD 14
               CXX_STANDARD_REQUIRED ON
               )

target_include_directories(usbcdc PRIVATE include PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(usbcdc PUBLIC barobo::cxx-util ${Boost_LIBRARIES})

if(WIN32)
    target_link_libraries(usbcdc PRIVATE setupapi)
elseif(APPLE)
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

file(WRITE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}-targets.cmake\")"
)

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
