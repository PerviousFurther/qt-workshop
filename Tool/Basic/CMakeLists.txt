
file(GLOB ToolBasic_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
    "*.cpp" "*.hpp" "*.h"
)

set(ToolBasic_RES_PREFIX "ToolBasic")

QT_ADD_LIBRARY(RingAppToolBasic SHARED ${ToolBasic_SRCS})

target_include_directories(RingAppToolBasic PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(RingAppToolBasic PRIVATE
    Qt6::Core Qt6::Widgets
)

set_target_properties(RingAppToolBasic PROPERTIES
    OUTPUT_NAME "Tools.Basic"
)

target_compile_definitions(RingAppToolBasic PRIVATE
    RINGAPP_TOOL_BASIC_EXPORT
    QML_MODULE_NAME="Tools.Basic"
    QML_MODULE_MAJOR=1
    QML_MODULE_MINOR=0
)

