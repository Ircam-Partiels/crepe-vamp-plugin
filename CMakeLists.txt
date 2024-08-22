cmake_minimum_required(VERSION 3.18)

set(IGNORE_VAMP_PLUGIN_TESTER OFF CACHE STRING "Disables the tests with vamp plugin tester")
set(PARTIELS_EXE_HINT_PATH "/Applications" CACHE PATH "")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Default value for MSVC_RUNTIME_LIBRARY of targets" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE CACHE BOOL "Default value for POSITION_INDEPENDENT_CODE of targets" FORCE)

### Include Vamp (IVE) ###
set(IVE_BUILD_HOST_LIBRARY OFF)
set(IVE_BUILD_PLUGIN_LIBRARY ON)
add_subdirectory(ircam-vamp-extension EXCLUDE_FROM_ALL)

### Include TensorFlow ###
set(SYSTEM_FARMHASH OFF)
set(SYSTEM_PTHREADPOOL OFF)
set(TFLITE_ENABLE_XNNPACK ON)
set(TFLITE_ENABLE_RUY ON)
set(BUILD_SHARED_LIBS OFF)
set(ABSL_PROPAGATE_CXX_STD ON)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS_BACKUP "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "" FORCE)
add_subdirectory(tensorflow/tensorflow/lite EXCLUDE_FROM_ALL)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ${CMAKE_SUPPRESS_DEVELOPER_WARNINGS_BACKUP} CACHE INTERNAL "" FORCE)
set_target_properties(tensorflow-lite PROPERTIES CXX_STANDARD 20)
set_target_properties(XNNPACK PROPERTIES CXX_STANDARD 20)

if(APPLE)
  target_compile_options(pthreadpool PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(eight_bit_int_gemm PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(farmhash PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(microkernels-prod PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(jit PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(operators PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(operator-run PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(operator-utils PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(subgraph PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(XNNPACK PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(cpuinfo PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_prepacked_cache PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_thread_pool PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_ctx PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_context PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_context_get_ctx PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_trmul PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_prepare_packed_matrices PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_pack_arm PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(ruy_frontend PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(indirection PRIVATE -Wno-shorten-64-to-32)
  target_compile_options(tensorflow-lite PRIVATE -Wno-shorten-64-to-32)
endif()

### Project ###
project(CrepeVampPlugin VERSION 1.0.0 LANGUAGES C CXX)

### Version ###
execute_process(COMMAND git log -1 --format=%h WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git status --porcelain WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE GIT_HAS_DIFF OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT GIT_HAS_DIFF STREQUAL "")
  set(GIT_COMMIT_ID "${GIT_COMMIT_ID}-head")
endif()

if(NOT CVP_BUILD_TAG OR CVP_BUILD_TAG STREQUAL "")
  set(CVP_BUILD_TAG ${CMAKE_PROJECT_VERSION})
endif()
message(STATUS "Crepe Vamp Plugin: " ${CVP_BUILD_TAG} " (" ${GIT_COMMIT_ID} ")")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt ${CVP_BUILD_TAG})

### Package ###
set(VPP_NAME "Crepe" CACHE PATH "The name of the project" FORCE)
set(VPP_COMPANY "Ircam" CACHE PATH "The name of the company" FORCE)
set(VPP_URL "www.ircam.fr" CACHE PATH "The URL of the project")
set(VPP_APPID "5aca3044-a39f-476d-b16c-ea033dbd1c3e" CACHE PATH "The unique ID of the project")
set(VPP_BUILD_TAG "${CVP_BUILD_TAG}" CACHE STRING "The current tag of the project" FORCE)
set(VPP_ABOUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resource/About.txt" CACHE PATH "The about file to add to the distribution" FORCE)
set(VPP_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resource/ChangeLog.txt" CACHE PATH "The changelog file to add to the distribution" FORCE)
set(VPP_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resource/Ircam-logo-noir-RS.png" CACHE PATH "The icon file to add to the distribution" FORCE)
set(VPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/package CACHE PATH "The directory of the package" FORCE)
include(vamp-plugin-packager/vamp-plugin-packager.cmake)

### Source ###
set(CVP_MODEL_H ${CMAKE_CURRENT_SOURCE_DIR}/source/cvp_model.h)


file(GLOB CVP_MODEL_CPP_LIST ${CMAKE_CURRENT_BINARY_DIR}/source/*.cpp)
list(LENGTH CVP_MODEL_CPP_LIST CVP_MODEL_CPP_LIST_SIZE)
if(NOT CVP_MODEL_CPP_LIST_SIZE EQUAL 5)
  set(CVP_MODEL_CPP_LIST_SIZE)
  file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/source")

  file(RELATIVE_PATH CVP_MODEL_HREL "${CMAKE_CURRENT_BINARY_DIR}/source" ${CVP_MODEL_H})

  foreach(CVP_MODEL_CAPACITY IN ITEMS "tiny" "small" "medium" "large" "full")
    set(CVP_MODEL_CPP ${CMAKE_CURRENT_BINARY_DIR}/source/cvp_model-${CVP_MODEL_CAPACITY}.cpp)
    file(WRITE ${CVP_MODEL_CPP} "#include \"${CVP_MODEL_HREL}\"\n\n")

    message(STATUS "Downloading model crepe-${CVP_MODEL_CAPACITY}.tflite")
    set(CVP_MODEL_PATH "${CMAKE_CURRENT_BINARY_DIR}/models/crepe-${CVP_MODEL_CAPACITY}.tflite")
    file(DOWNLOAD "https://github.com/Ircam-Partiels/crepe-vamp-plugin/releases/download/0.0.0/crepe-${CVP_MODEL_CAPACITY}.tflite" ${CVP_MODEL_PATH} STATUS DOWNLOAD_STATUS)
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
    if(${STATUS_CODE} EQUAL 0)
      message(STATUS "Download crepe-${CVP_MODEL_CAPACITY}.tflite completed successfully!")
    else()
      message(FATAL_ERROR "Error occurred during download: ${ERROR_MESSAGE}")
    endif()

    message(STATUS "Generating data model crepe-${CVP_MODEL_CAPACITY}.tflite")
    file(READ ${CVP_MODEL_PATH} CVP_HEX_DATA HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," CVP_HEX_DATA ${CVP_HEX_DATA})
    file(APPEND ${CVP_MODEL_CPP} "namespace Cvp\n{\n")
    file(APPEND ${CVP_MODEL_CPP} "static const unsigned char binary_data_${CVP_MODEL_CAPACITY}[] =\n{\n")
    file(APPEND ${CVP_MODEL_CPP} "${CVP_HEX_DATA}")
    file(APPEND ${CVP_MODEL_CPP} "};\n}\n\n")
    file(APPEND ${CVP_MODEL_CPP} "const void* Cvp::model_${CVP_MODEL_CAPACITY} = (const void*)Cvp::binary_data_${CVP_MODEL_CAPACITY};\n")
    file(APPEND ${CVP_MODEL_CPP} "const size_t Cvp::model_${CVP_MODEL_CAPACITY}_size = sizeof(Cvp::binary_data_${CVP_MODEL_CAPACITY});\n")
    file(APPEND ${CVP_MODEL_CPP} "\n")
  endforeach()

  file(GLOB CVP_MODEL_CPP_LIST ${CMAKE_CURRENT_BINARY_DIR}/source/*.cpp)
endif()

file(GLOB CVP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/cvp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/cvp.h
  ${CVP_MODEL_H}
)
source_group("sources" FILES ${CVP_SOURCES})

### Target ###
add_library(cvp SHARED ${CVP_SOURCES} ${CVP_MODEL_CPP_LIST} ${CVP_MODEL_H})
ive_prepare_plugin_target(cvp)
target_compile_definitions(cvp PRIVATE CVP_PLUGIN_VERSION=${PROJECT_VERSION_MAJOR})
target_link_libraries(cvp PRIVATE tensorflow-lite)

add_custom_command(TARGET cvp POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resource/crepe.cat "$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_BINARY_DIR}/Debug/crepe.cat,${CMAKE_CURRENT_BINARY_DIR}/Release/crepe.cat>")
set_target_properties(cvp PROPERTIES LIBRARY_OUTPUT_NAME crepe)
vpp_add_plugin(cvp)

find_program(PARTIELS_EXE "Partiels" HINTS ${PARTIELS_EXE_HINT_PATH} NO_CACHE)
if(PARTIELS_EXE)
  if(NOT IS_DIRECTORY ${PARTIELS_EXE}) 
      cmake_path(GET PARTIELS_EXE PARENT_PATH PARTIELS_EXE_PARENT)
      cmake_path(SET PARTIELS_EXE NORMALIZE "${PARTIELS_EXE_PARENT}/../../")
  endif()
  message(STATUS "Debugging: " ${PARTIELS_EXE})
  set_target_properties(cvp PROPERTIES XCODE_SCHEME_EXECUTABLE ${PARTIELS_EXE})
  set_target_properties(cvp PROPERTIES XCODE_SCHEME_ARGUMENTS "${CMAKE_CURRENT_SOURCE_DIR}/test/crepe.ptldoc")
  set_target_properties(cvp PROPERTIES XCODE_SCHEME_ENVIRONMENT "VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Debug")
endif()

### Format ###
find_program(CLANG_FORMAT_EXE "clang-format" HINTS "C:/Program Files/LLVM/bin")
if(CLANG_FORMAT_EXE)
    add_custom_target(cvp_check_format ${CLANG_FORMAT_EXE} --Werror --dry-run --verbose -style=file ${CVP_SOURCES})
    add_custom_target(cvp_apply_format ${CLANG_FORMAT_EXE} -i -style=file ${CVP_SOURCES})
else()
    message(STATUS "Clang Format targets cannot be generated because clang-format is not found")
endif()

### Manual ###
set(CVP_MANUAL_DIR ${CMAKE_CURRENT_BINARY_DIR}/Manual)
file(MAKE_DIRECTORY ${CVP_MANUAL_DIR})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/docs/crepe-manual.md MANUAL_CONTENT)
string(REPLACE "APPVERSION" "${CVP_BUILD_TAG} (${GIT_COMMIT_ID})" MANUAL_CONTENT ${MANUAL_CONTENT})
string(REPLACE "src=\"../resource/" "src=\"${CMAKE_CURRENT_SOURCE_DIR}/resource/" MANUAL_CONTENT ${MANUAL_CONTENT})
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/crepe-manual.md ${MANUAL_CONTENT})
find_program(MDPDF_EXE "mdpdf")
if(MDPDF_EXE)
  add_custom_target(cvp_manual COMMAND ${MDPDF_EXE} ${CMAKE_CURRENT_BINARY_DIR}/crepe-manual.md ${CVP_MANUAL_DIR}/crepe-manual.pdf VERBATIM)
else()
  message(STATUS "Manual target cannot be generated because mdpdf is not found")
endif()

### Publishing ###
set(PROJECT_URL "https://github.com/Ircam-Partiels/crepe-vamp-plugin")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "## Downloads\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-Linux.tar.gz](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-Linux.tar.gz)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-MacOS.pkg](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-MacOS.pkg)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-Windows.exe](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-Windows.exe)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-Manual.zip](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-Manual.zip)\n") 

### Install ###
if(APPLE)
  install(TARGETS cvp RUNTIME LIBRARY DESTINATION "~/Library/Audio/Plug-Ins/Vamp/")
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/crepe.cat DESTINATION "~/Library/Audio/Plug-Ins/Vamp/")
elseif(UNIX)
  install(TARGETS cvp RUNTIME LIBRARY DESTINATION "~/vamp/")
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/crepe.cat DESTINATION "~/vamp/")
elseif(WIN32)
  install(TARGETS cvp RUNTIME DESTINATION "$ENV{PROGRAMFILES}/Vamp Plugins/" PERMISSIONS OWNER_WRITE)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/crepe.cat DESTINATION "$ENV{PROGRAMFILES}/Vamp Plugins/")
endif()

### Testing ###
if(NOT IGNORE_VAMP_PLUGIN_TESTER)
  enable_testing()
  if(APPLE)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester)
      file(DOWNLOAD "https://github.com/pierreguillot/vamp-plugin-tester/releases/download/1.1/vamp-plugin-tester-1.1-osx-arm.zip" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  elseif(UNIX)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester)
      file(DOWNLOAD "https://github.com/pierreguillot/vamp-plugin-tester/releases/download/1.1/vamp-plugin-tester-1.1-linux64.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.tar.gz" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  elseif(WIN32)
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester.exe)
      file(DOWNLOAD "https://github.com/pierreguillot/vamp-plugin-tester/releases/download/1.1/vamp-plugin-tester-1.1a-win64.zip" "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.zip")
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester.zip" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  endif()

  add_test(NAME VampPluginTester COMMAND ${CMAKE_CURRENT_BINARY_DIR}/vamp-plugin-tester/vamp-plugin-tester -a)
  set_tests_properties(VampPluginTester PROPERTIES ENVIRONMENT "$<IF:$<CONFIG:Debug>,VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Debug,VAMP_PATH=${CMAKE_CURRENT_BINARY_DIR}/Release>")
endif()
