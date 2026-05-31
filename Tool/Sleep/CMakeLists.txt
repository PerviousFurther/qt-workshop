
file(GLOB ToolSleep_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
    "*.cpp" "*.hpp" "*.h"
)

set(ToolSleep_RES_PREFIX "ToolSleep")

QT_ADD_LIBRARY(RingAppToolSleep SHARED ${ToolSleep_SRCS})

target_include_directories(RingAppToolSleep PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(RingAppToolSleep PRIVATE
    RingAppCore RingAppBackendDevice RingAppGuiUi RingAppDriverBasic
    RingAppDriverUiTest RingAppToolBasic RingAppBackendNetwork 
    Qt6::Widgets 
)

set_target_properties(RingAppToolSleep PROPERTIES
    OUTPUT_NAME "Tools.Sleep"
)

target_compile_definitions(RingAppToolSleep PRIVATE
    RINGAPP_TOOL_SLEEP_EXPORT
    QML_MODULE_NAME="Tools.Sleep"
    QML_MODULE_MAJOR=1
    QML_MODULE_MINOR=0
)

