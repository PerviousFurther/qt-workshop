set(RINGAPP_DEVICE_QT_COMPONENTS Bluetooth Core)
if(RINGAPP_ENABLE_GUI)
    list(APPEND RINGAPP_DEVICE_QT_COMPONENTS Qml)
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS ${RINGAPP_DEVICE_QT_COMPONENTS})

file(GLOB BACKEND_DEVICE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB BACKEND_DEVICE_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

QT_ADD_LIBRARY(RingAppBackendDevice SHARED ${BACKEND_DEVICE_SRCS} ${BACKEND_DEVICE_HDRS})

target_include_directories(RingAppBackendDevice PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(RingAppBackendDevice PRIVATE
    RingAppCore
    Qt6::Bluetooth
    Qt6::Core
    Qt6::Network
)

set_target_properties(RingAppBackendDevice PROPERTIES
    OUTPUT_NAME "Backend.Device"
)

target_compile_definitions(RingAppBackendDevice PRIVATE 
    RINGAPP_BACKEND_DEVICE_EXPORT
    QML_MODULE_NAME="Backend.Device"
    QML_MODULE_MAJOR=0
    QML_MODULE_MINOR=0
)


