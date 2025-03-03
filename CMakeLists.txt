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
find_package(Python COMPONENTS Interpreter)
if(NOT Python_FOUND)
  message(FATAL_ERROR "Python not found: please install Python that is required by TensorFlow")
endif()

set(SYSTEM_FARMHASH OFF)
set(SYSTEM_PTHREADPOOL OFF)
set(TFLITE_ENABLE_XNNPACK ON)
set(TFLITE_ENABLE_RUY ON)
set(BUILD_SHARED_LIBS OFF)
set(ABSL_PROPAGATE_CXX_STD ON)

add_subdirectory(tensorflow/tensorflow/lite EXCLUDE_FROM_ALL)
set_target_properties(tensorflow-lite PROPERTIES CXX_STANDARD 20)
set_target_properties(XNNPACK PROPERTIES CXX_STANDARD 20)

if(APPLE)
  add_dependencies(profiling_info_proto model_runtime_info_proto)
  foreach(TFLIB IN ITEMS kleidiai pthreadpool eight_bit_int_gemm farmhash microkernels-prod microkernel-utils microparams-init operators operator-run operator-utils subgraph xnnpack-delegate XNNPACK cpuinfo ruy_prepacked_cache ruy_thread_pool ruy_ctx ruy_context ruy_context_get_ctx ruy_trmul ruy_prepare_packed_matrices ruy_pack_arm ruy_frontend indirection tensorflow-lite normalization packing cache memory reference-ukernels datatype)
    if(TARGET ${TFLIB})
        target_compile_options(${TFLIB} PRIVATE -Wno-shorten-64-to-32)
    endif()
  endforeach()
  if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" AND NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    target_compile_definitions(xnnpack-delegate PUBLIC TF_LITE_DISABLE_X86_NEON)
  endif()
  target_compile_options(tensorflow-lite PRIVATE -Wno-deprecated-this-capture)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(ruy_pack_arm PRIVATE -Wno-comment)
  target_compile_options(ruy_kernel_avx512 PRIVATE -Wno-unknown-pragmas)
  target_compile_options(xnnpack-delegate PRIVATE -Wno-deprecated-declarations)
  target_compile_options(tensorflow-lite PRIVATE -Wno-deprecated -Wno-deprecated-declarations -Wno-return-type)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(xnnpack-delegate PRIVATE /wd4267 /wd4273 /wd4834)
  target_compile_options(XNNPACK PRIVATE /wd4098 /wd4190 /wd4244)
  target_compile_options(farmhash PRIVATE /wd4319)
  target_compile_options(flatc PRIVATE /wd4530)
  target_compile_options(operator-run PRIVATE /wd4098)
  target_compile_options(packing PRIVATE /wd4190 /wd4244)
  target_compile_options(reference-ukernels PRIVATE /wd4190)
  target_compile_options(ruy_kernel_avx512 PRIVATE /wd4068)
  target_compile_options(tensorflow-lite PRIVATE /wd4068 /wd4098 /wd4715 /wd4804 /wd4996)
  target_link_options(tensorflow-lite PRIVATE /wd4006)
endif()

### Project ###
project(CrepeVampPlugin VERSION 2.0.0 LANGUAGES C CXX)

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

set_target_properties(cvp PROPERTIES LIBRARY_OUTPUT_NAME ircamcrepe)
vpp_add_plugin(cvp ${CMAKE_CURRENT_SOURCE_DIR}/resource/ircamcrepe.cat)
vpp_set_plugin_debug(cvp "${CMAKE_CURRENT_SOURCE_DIR}/test/crepe.ptldoc")
vpp_set_plugin_install(cvp)
vpp_create_clang_format_targets(cvp ${CVP_SOURCES})

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