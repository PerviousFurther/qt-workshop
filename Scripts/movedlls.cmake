# if (WIN32)
#     set(EXE_POSTFIX_ ".exe")
#     set(PLATFORM_PREFIX "win")
# else()
#     set(EXE_POSTFIX_ "")
#     set(PLATFORM_PREFIX "android")
# endif()
# 
# set(DEPLOYQT_EXECUTABLE "$ENV{QTDIR}/bin/${PLATFORM_PREFIX}deployqt${EXE_POSTFIX_}")
# if(EXISTS "${DEPLOYQT_EXECUTABLE}")
#     message(STATUS "Found deployment tool: ${DEPLOYQT_EXECUTABLE}")
#     execute_process(
#         COMMAND ${DEPLOYQT_EXECUTABLE} 
#             "${CMAKE_INSTALL_PREFIX}/RingApp${EXE_POSTFIX_}"
#             "--qmldir" "${CMAKE_INSTALL_PREFIX}/../../../"
#             "--plugindir" "${CMAKE_INSTALL_PREFIX}/temp"
#             "--libdir" "${CMAKE_INSTALL_PREFIX}/temp"
#             "--no-translations" "--force"
#             "--no-compiler-runtime" "--no-opengl-sw"
#         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
#         RESULT_VARIABLE DEPLOY_RESULT
#         OUTPUT_VARIABLE DEPLOY_OUTPUT
#         ERROR_VARIABLE  DEPLOY_ERROR
#     )
#     if(NOT ${DEPLOY_RESULT} EQUAL 0)
#         message(FATAL_ERROR "deploy qt ${CMAKE_INSTALL_PREFIX}/RingApp${EXE_POSTFIX_} failed: result: ${DEPLOY_RESULT} ;;; error: ${DEPLOY_ERROR}")
#     endif()
# else()
#     message(FATAL_ERROR "deploy at ${DEPLOYQT_EXECUTABLE} not found.")
# endif()

# find_program(QML_IMPORT_SCANNER  NAMES $ENV{QTDIR}/bin/qmlimportscanner${EXE_POSTFIX_})
# if(QML_IMPORT_SCANNER)
#     message(${CMAKE_CURRENT_SOURCE_DIR}/../)
#     execute_process(
#         COMMAND ${QML_IMPORT_SCANNER} "${CMAKE_SOURCE_DIR}/../../../"
#         OUTPUT_VARIABLE QML_DEPS_JSON
#     )
#     message(STATUS "QML Dependencies JSON:\n${QML_DEPS_JSON}")
# else()
#     message(WARNING "QML import scanner not found!")
# endif()

# set(REAL_PREFIX "${CMAKE_INSTALL_PREFIX}")
# set(BIN_DIR "${REAL_PREFIX}/bin")
# set(DEST_DIR "${REAL_PREFIX}")
# file(GLOB DLL_FILES "${BIN_DIR}/*.dll")
# if(DLL_FILES)
#     message(STATUS "Moving DLLs to root: ${DLL_FILES}")
#     foreach(DLL IN LISTS DLL_FILES)
#         message(STATUS " Moving ${DLL}")
#         file(COPY "${DLL}" DESTINATION "${DEST_DIR}")
#         file(REMOVE "${DLL}")
#     endforeach()
# endif()

message(STATUS "Move temp..")
file(COPY "${CMAKE_INSTALL_PREFIX}/temp/" 
	 DESTINATION "${CMAKE_INSTALL_PREFIX}")
file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/temp")
message(STATUS "Success!")