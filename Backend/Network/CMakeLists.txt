file(GLOB BACKEND_NETWORK_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB BACKEND_NETWORK_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)


QT_ADD_LIBRARY(RingAppBackendNetwork SHARED ${BACKEND_NETWORK_SRCS} ${BACKEND_NETWORK_HDRS})

target_include_directories(RingAppBackendNetwork PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(RingAppBackendNetwork PRIVATE
    RingAppCore
    Qt6::Core
    Qt6::Network
)

set_target_properties(RingAppBackendNetwork PROPERTIES
    OUTPUT_NAME "Backend.Network"
)

target_compile_definitions(RingAppBackendNetwork PRIVATE 
    RINGAPP_BACKEND_NETWORK_EXPORT
    QML_MODULE_NAME="Backend.Network" 
    QML_MODULE_MAJOR=0 
    QML_MODULE_MINOR=0
)




