file(GLOB DRIVER_BASIC_DRIVER_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB DRIVER_BASIC_DRIVER_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

qt_add_library(RingAppDriverBasic SHARED 
    ${DRIVER_BASIC_DRIVER_SRCS} 
    ${DRIVER_BASIC_DRIVER_HDRS}
)

target_include_directories(RingAppDriverBasic PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

find_package(Qt6 6.4 REQUIRED COMPONENTS Bluetooth)

target_link_libraries(RingAppDriverBasic PRIVATE
    RingAppCore
    RingAppBackendNetwork
    RingAppBackendDevice
    Qt6::Core
)

set_target_properties(RingAppDriverBasic PROPERTIES
    OUTPUT_NAME "Driver.Basic"
)

target_compile_definitions(RingAppDriverBasic PRIVATE
    RINGAPP_DRIVER_BASIC_EXPORT
    QML_MODULE_NAME="Driver.Basic"
    QML_MODULE_MAJOR=1
    QML_MODULE_MINOR=0
)
