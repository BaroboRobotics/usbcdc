project(usbcdc-tests LANGUAGES CXX)

##############################################################################
# Tests

set(testSources
    monitor-test.cpp
)

add_executable(usbcdc-test main.cpp ${testSources})
target_link_libraries(usbcdc-test PRIVATE usbcdc)
add_test(NAME usbcdc-test COMMAND usbcdc-test)
