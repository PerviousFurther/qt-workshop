file(GLOB GUIAPP_UI_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB GUIAPP_UI_HDRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
file(GLOB_RECURSE GUIAPP_UI_RES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
    "*.svg"
)

QT_ADD_LIBRARY(RingAppGuiUi
    SHARED
        ${GUIAPP_UI_SRCS}
        ${GUIAPP_UI_HDRS}
)
QT_ADD_RESOURCES(RingAppGuiUi GuiAppUi FILES ${GUIAPP_UI_RES})

target_link_libraries(RingAppGuiUi PRIVATE
    RingAppCore RingAppBackendDevice
    RingAppBackendNetwork RingAppBackendAiChat
    Qt6::Core Qt6::Gui Qt6::Widgets
)

target_include_directories(RingAppGuiUi PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

set_target_properties(RingAppGuiUi PROPERTIES
    OUTPUT_NAME "App.Ui"
)

target_compile_definitions(RingAppGuiUi PRIVATE 
    RINGAPP_GUI_UI_EXPORT
    QML_MODULE_NAME="App.Ui"
    QML_MODULE_MAJOR=0
    QML_MODULE_MINOR=0
    QML_RESOURCE_PREFIX="GuiAppUi"
    RESOURCE_NAME=GuiAppUi
)



