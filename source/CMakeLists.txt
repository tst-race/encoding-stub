set(PLUGIN_NAME PluginStubEncoding)
set(PLUGIN_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../kit")

if(ANDROID)
    if("${TARGET_ARCHITECTURE}" STREQUAL "ANDROID_arm64-v8a")
        set(KIT_DIRS android-arm64-v8a-client)
    else()
        set(KIT_DIRS android-x86_64-client)
    endif()
else()
    if("${TARGET_ARCHITECTURE}" STREQUAL "LINUX_arm64-v8a")
        set(NODE_TYPE linux-arm64-v8a)
    else()
        set(NODE_TYPE linux-x86_64)
    endif()
    set(KIT_DIRS ${NODE_TYPE}-client ${NODE_TYPE}-server)
endif()

add_library(${PLUGIN_NAME} SHARED
    encoding/PluginStubEncoding.cpp
    encoding/base64.cpp
)
target_include_directories(${PLUGIN_NAME} PRIVATE encoding)
target_compile_definitions(${PLUGIN_NAME} PRIVATE BUILD_VERSION="${BUILD_VERSION}")
set_target_properties(${PLUGIN_NAME} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    NO_SONAME ON
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/race/comms/${PLUGIN_NAME})
install(FILES manifest.json DESTINATION lib/race/comms/${PLUGIN_NAME})

foreach(KIT_DIR ${KIT_DIRS})
    set(KIT_PATH "${PLUGIN_BASE_DIR}/artifacts/${KIT_DIR}/${PLUGIN_NAME}")
    add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${KIT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PLUGIN_NAME}>" "${KIT_PATH}/"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json" "${KIT_PATH}/manifest.json"
        VERBATIM
    )
endforeach()