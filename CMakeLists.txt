cmake_minimum_required(VERSION 3.2.1)
project(usbcdc VERSION 0.1.0)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.59.0 REQUIRED COMPONENTS system filesystem)

if(APPLE)
    add_subdirectory(CFPP)
endif()

set(SOURCES src/devices.cpp)

add_library(usbcdc STATIC ${SOURCES})

set_target_properties(usbcdc
    PROPERTIES CXX_STANDARD 11
               CXX_STANDARD_REQUIRED ON
               )

target_include_directories(usbcdc PUBLIC include ${Boost_INCLUDE_DIRS})
target_link_libraries(usbcdc PUBLIC ${Boost_LIBRARIES})

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
