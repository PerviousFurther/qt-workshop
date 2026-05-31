file(GLOB BACKEND_AiChat_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB BACKEND_AiChat_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

QT_ADD_LIBRARY(RingAppBackendAiChat SHARED ${BACKEND_AiChat_SRCS} ${BACKEND_AiChat_HDRS})

target_include_directories(RingAppBackendAiChat PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(RingAppBackendAiChat PRIVATE
    RingAppCore RingAppBackendNetwork
    Qt6::Core Qt6::Network
)

set_target_properties(RingAppBackendAiChat PROPERTIES
    OUTPUT_NAME "Backend.AiChat"
)

target_compile_definitions(RingAppBackendAiChat PRIVATE 
    RINGAPP_BACKEND_AICHAT_EXPORT
    QML_MODULE_NAME="Backend.AiChat"
    QML_MODULE_MAJOR=0
    QML_MODULE_MINOR=0
)


