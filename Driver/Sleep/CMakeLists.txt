file(GLOB DRIVER_SLEEP_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB DRIVER_SLEEP_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

message("Driver ${DRIVER_SLEEP_SRCS}")

QT_ADD_LIBRARY(RingAppDriverSleep SHARED
    ${DRIVER_SLEEP_SRCS} ${DRIVER_SLEEP_HDRS}
)

target_include_directories(RingAppDriverSleep PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

find_package(Qt6 6.4 REQUIRED COMPONENTS Bluetooth)

target_link_libraries(RingAppDriverSleep PRIVATE
    RingAppCore
    RingAppDriver
    Qt6::Core
    Qt6::Bluetooth
)

set_target_properties(RingAppDriverSleep PROPERTIES
    OUTPUT_NAME "Driver.Sleep"
)

target_compile_definitions(RingAppDriverSleep PRIVATE
    RINGAPP_DRIVER_SLEEP_EXPORT
    QML_MODULE_NAME="Driver.Sleep" 
    QML_MODULE_MAJOR=0 
    QML_MODULE_MINOR=0
)


