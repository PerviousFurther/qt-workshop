file(GLOB UI_TEST_DRIVER_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB UI_TEST_DRIVER_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

qt_add_library(RingAppDriverUiTest SHARED 
    ${UI_TEST_DRIVER_SRCS} 
    ${UI_TEST_DRIVER_HDRS}
)

target_include_directories(RingAppDriverUiTest PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

find_package(Qt6 6.4 REQUIRED COMPONENTS Bluetooth)

target_link_libraries(RingAppDriverUiTest PRIVATE
    RingAppCore
    RingAppBackendNetwork
    RingAppBackendDevice
    RingAppGuiUi
    RingAppDriverBasic
    Qt6::Core
    Qt6::Bluetooth
    Qt6::Widgets
)

set_target_properties(RingAppDriverUiTest PROPERTIES
    OUTPUT_NAME "Driver.UiTest"
)

target_compile_definitions(RingAppDriverUiTest PRIVATE
    RINGAPP_DRIVER_UI_TEST_EXPORT
    QML_MODULE_NAME="Driver.UiTest"
    QML_MODULE_MAJOR=1
    QML_MODULE_MINOR=0
)
