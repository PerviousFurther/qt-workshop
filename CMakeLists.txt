cmake_minimum_required(VERSION 3.21.1)

project(RingApp LANGUAGES CXX)

option(RINGAPP_ENABLE_GUI "Enable GUI/QML support" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:preprocessor)
    add_link_options(/INCREMENTAL:NO)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    message("Set CMAKE_BUILD_TYPE as Debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# if(CAMKE_BUILD_TYPE STREQUAL "Debug")
#     add_compile_definitions(QT_QML_DEBUG)
# endif()

# Enable Qt automoc/autorcc for QML and resources
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CONFIGURATION_TYPES)
    set(RINGAPP_CONFIGURATION_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Out/$<CONFIG>")    
else()
    set(RINGAPP_CONFIGURATION_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Out/${CMAKE_BUILD_TYPE}")
endif()

set(CMAKE_BINARY_DIR                    "${RINGAPP_CONFIGURATION_OUTPUT_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY      "${RINGAPP_CONFIGURATION_OUTPUT_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY          "${RINGAPP_CONFIGURATION_OUTPUT_DIR}/pdb")
set(CMAKE_INSTALL_PREFIX                "${RINGAPP_CONFIGURATION_OUTPUT_DIR}/install")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY      "${RINGAPP_CONFIGURATION_OUTPUT_DIR}/install")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(WARNING "In-source build is discouraged. Consider using a separate build directory.")
endif()

if(RINGAPP_ENABLE_GUI)
    add_compile_definitions(RINGAPP_ENABLE_GUI=1)
    set(RINGAPP_QT_COMPONENTS Core Gui Widgets Bluetooth Network)
else()
    add_compile_definitions(RINGAPP_ENABLE_GUI=0)
    set(RINGAPP_QT_COMPONENTS Core Bluetooth Network)
endif()

# Find common Qt components used by the application and modules
find_package(Qt6 COMPONENTS ${RINGAPP_QT_COMPONENTS} REQUIRED)
qt_standard_project_setup()

# qt_policy(SET QTP0001 NEW)
# qt_policy(SET QTP0004 NEW)
# qt_policy(SET QTP0005 NEW)

include(GNUInstallDirs)
add_subdirectory(Core)
add_subdirectory(Backend)
add_subdirectory(Driver)

if(RINGAPP_ENABLE_GUI)
    add_subdirectory(GuiApp)
endif()

add_subdirectory(Tool)

install(TARGETS RingAppGui
    BUNDLE  DESTINATION "${CMAKE_INSTALL_PREFIX}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

if(ANDROID)
    set(deploy_tool_options_arg
        # Android deploy options — androiddeployqt handles most automatically
    )
else()
    set(deploy_tool_options_arg
        --plugindir "${CMAKE_INSTALL_PREFIX}/temp"
        --libdir "${CMAKE_INSTALL_PREFIX}/temp"
        # --qml-deploy-dir "${CMAKE_INSTALL_PREFIX}/temp/qml"
        # --qmldir "${CMAKE_SOURCE_DIR}/Qmls"
        --no-ffmpeg --no-opengl-sw
    )
    if(APPLE)
        set(deploy_tool_options_arg --hardened-runtime ${deploy_tool_options_arg})
    elseif(WIN32)
        set(deploy_tool_options_arg --no-system-d3d-compiler ${deploy_tool_options_arg})
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(deploy_tool_options_arg --debug ${deploy_tool_options_arg})
else()
    set(deploy_tool_options_arg --release ${deploy_tool_options_arg})
endif()

qt_generate_deploy_app_script(TARGET RingAppGui
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_COMPILER_RUNTIME
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
)
install(SCRIPT ${deploy_script})

if(WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DLL_PREFIX "d")
    else()
        set(DLL_PREFIX "")
    endif()

    install(
        FILES
            "$ENV{QTDIR}/bin/Qt6Bluetooth${DLL_PREFIX}.dll"
        DESTINATION "${CMAKE_INSTALL_PREFIX}")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/Scripts/movedlls.cmake")
endif()
