find_package(GTest REQUIRED CONFIG)
find_library(LIB_RACE_SDK_COMMON raceSdkCommon REQUIRED)

add_executable(encodingStubTests
    TestPluginStubEncoding.cpp
    ../../source/encoding/PluginStubEncoding.cpp
    ../../source/encoding/base64.cpp
)
target_include_directories(encodingStubTests PRIVATE ../../source/encoding)
target_compile_definitions(encodingStubTests PRIVATE BUILD_VERSION="${BUILD_VERSION}")
target_link_libraries(encodingStubTests PRIVATE
    GTest::gmock
    GTest::gtest_main
    ${LIB_RACE_SDK_COMMON}
)

if(NOT EXISTS "${RACEBOAT_SOURCE_DIR}/test/common/MockEncodingSdk.h")
    message(FATAL_ERROR "Set RACEBOAT_SOURCE_DIR to the Raceboat checkout for MockEncodingSdk.h")
endif()
target_include_directories(encodingStubTests PRIVATE
    "${RACEBOAT_SOURCE_DIR}/test/common"
    "${RACEBOAT_SOURCE_DIR}/include"
)

add_test(NAME encodingStubTests COMMAND encodingStubTests)