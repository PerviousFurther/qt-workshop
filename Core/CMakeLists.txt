# Module static library

file(GLOB CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB CORE_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

message("Core contain: ${CORE_SRCS}")

QT_ADD_LIBRARY(RingAppCore 
    SHARED 
    ${CORE_SRCS} 
    ${CORE_HDRS}
)

target_include_directories(RingAppCore PUBLIC
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(RingAppCore PUBLIC Qt6::Core)

set_target_properties(RingAppCore PROPERTIES
    OUTPUT_NAME "App.Core"
)

target_compile_definitions(RingAppCore PRIVATE 
    RINGAPP_CORE_EXPORT
    QML_MODULE_NAME="App.Core" 
    QML_MODULE_MAJOR=1 
    QML_MODULE_MINOR=0
)
