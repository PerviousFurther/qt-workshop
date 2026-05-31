# Ui module as shared library
file(GLOB UI_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# add_subdirectory(Ui)

qt_add_executable(RingAppGui
    MANUAL_FINALIZATION    
    ${UI_SRCS}
)
# qt_import_qml_plugins(RingAppGui)

target_include_directories(RingAppGui PRIVATE
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(RingAppGui PRIVATE
    RingAppCore Qt6::Core Qt6::Gui Qt6::Widgets
)

set_target_properties(RingAppGui PROPERTIES
    OUTPUT_NAME "RingApp"
)

# Should have terminal
# if (WIN32)
#     set_target_properties(RingAppGui PROPERTIES
#         WIN32_EXECUTABLE TRUE
#     )
# endif()

qt_finalize_executable(RingAppGui)


