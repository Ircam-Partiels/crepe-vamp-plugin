cmake_minimum_required(VERSION 3.18)

set(IGNORE_VAMP_PLUGIN_TESTER OFF CACHE STRING "Disables the tests with vamp plugin tester")
set(PARTIELS_EXE_HINT_PATH "/Applications" CACHE PATH "")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_XCODE_GENERATE_SCHEME ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Default value for MSVC_RUNTIME_LIBRARY of targets" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE CACHE BOOL "Default value for POSITION_INDEPENDENT_CODE of targets" FORCE)

### Project ###
project(CrepeVampPlugin VERSION 3.0.0 LANGUAGES C CXX)

### Models only ###
set(CVP_MODELS_ONLY OFF CACHE BOOL "Only generates the C++ model source files (used by the CI cache, without TensorFlow and the other dependencies)")

if(NOT CVP_MODELS_ONLY)
### Include Vamp (IVE) ###
set(IVE_BUILD_HOST_LIBRARY OFF)
set(IVE_BUILD_PLUGIN_LIBRARY ON)
add_subdirectory(ircam-vamp-extension EXCLUDE_FROM_ALL)

### Include TensorFlow ###
find_package(Python COMPONENTS Interpreter)
if(NOT Python_FOUND)
  message(FATAL_ERROR "Python not found: please install Python that is required by TensorFlow")
endif()

set(TENSORFLOW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tensorflow" CACHE PATH "" FORCE)
set(ABSL_MSVC_STATIC_RUNTIME ON CACHE BOOL "Link Abseil with the static MSVC runtime (/MT) like the rest of the project" FORCE)
set(TFLITE_ENABLE_BENCHMARK OFF)
set(TFLITE_ENABLE_TESTS OFF)
set(TFLITE_ENABLE_INSTALL OFF)

set(SYSTEM_FARMHASH OFF)
set(SYSTEM_PTHREADPOOL OFF)
set(TFLITE_ENABLE_XNNPACK ON)
set(TFLITE_ENABLE_RUY ON)
set(BUILD_SHARED_LIBS OFF)
set(ABSL_PROPAGATE_CXX_STD ON)

add_subdirectory(tensorflow/tensorflow/lite EXCLUDE_FROM_ALL)

set_target_properties(tensorflow-lite PROPERTIES CXX_STANDARD 20)
set_target_properties(XNNPACK PROPERTIES CXX_STANDARD 20)

### TensorFlow compiler settings (the compiler is only known after project()) ###
if(APPLE)
  add_dependencies(profiling_info_proto model_runtime_info_proto)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # model_building.cc is only used by TensorFlow tests and fails to compile with MSVC (C2248 on a friend class)
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/tensorflow/tensorflow/lite/core/model_building.cc TARGET_DIRECTORY tensorflow-lite PROPERTIES HEADER_FILE_ONLY ON)
endif()

foreach(TFLIB IN ITEMS absl_debugging_internal absl_base kleidiai absl_synchronization pthreadpool eight_bit_int_gemm farmhash cpuinfo XNNPACK xnnpack-delegate xnnpack-allocator xnnpack-base xnnpack-cache xnnpack-datatype xnnpack-fingerprint-cache xnnpack-fingerprint-check xnnpack-fingerprint-id xnnpack-hardware-config xnnpack-indirection xnnpack-logging xnnpack-memory xnnpack-microkernel-utils xnnpack-microkernels-all xnnpack-microkernels-prod xnnpack-microparams-init xnnpack-sanitizers xnnpack-mutex xnnpack-normalization xnnpack-operator-delete xnnpack-operator-run xnnpack-operator-utils xnnpack-operators xnnpack-pack-lh xnnpack-packing xnnpack-reference-ukernels xnnpack-subgraph ruy_prepacked_cache ruy_thread_pool ruy_profiler_instrumentation ruy_have_built_path_for_avx ruy_have_built_path_for_avx2_fma ruy_have_built_path_for_avx512 ruy_ctx ruy_context ruy_kernel_arm ruy_context_get_ctx ruy_trmul ruy_prepare_packed_matrices ruy_pack_arm ruy_frontend tensorflow-lite ruy_kernel_avx512 flatc flatbuffers-flatc)
  set(TFLIB_TYPE "")
  if(TARGET ${TFLIB})
    get_property(TFLIB_TYPE TARGET ${TFLIB} PROPERTY TYPE)
  endif()
  if(TFLIB_TYPE MATCHES "_LIBRARY$|EXECUTABLE" AND NOT TFLIB_TYPE STREQUAL "INTERFACE_LIBRARY")
    if(APPLE)
      target_compile_options(${TFLIB} PRIVATE -Wno-shorten-64-to-32 -Wno-deprecated-this-capture)
      set_target_properties(${TFLIB} PROPERTIES STATIC_LIBRARY_OPTIONS "-no_warning_for_no_symbols")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(${TFLIB} PRIVATE -Wno-comment -Wno-unknown-pragmas -Wno-deprecated -Wno-deprecated-declarations -Wno-return-type)  
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${TFLIB} PRIVATE /wd4006 /wd4068 /wd4098 /wd4190 /wd4244 /wd4267 /wd4273 /wd4319 /wd4530 /wd4715 /wd4804 /wd4834 /wd4996 /wd5287)
    endif()
  endif()
endforeach()
endif()

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

if(NOT CVP_MODELS_ONLY)
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
endif()

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

if(NOT CVP_MODELS_ONLY)
### Target ###
add_library(cvp MODULE ${CVP_SOURCES} ${CVP_MODEL_CPP_LIST} ${CVP_MODEL_H})
ive_prepare_plugin_target(cvp)
target_compile_definitions(cvp PRIVATE CVP_PLUGIN_VERSION=${PROJECT_VERSION_MAJOR})
target_link_libraries(cvp PRIVATE tensorflow-lite)

set_target_properties(cvp PROPERTIES LIBRARY_OUTPUT_NAME ircamcrepe)

if(APPLE)
  set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resource/ircamcrepe.cat")
  set_source_files_properties("${RESOURCE_FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(cvp PRIVATE "${RESOURCE_FILE}")

  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  target_link_libraries(cvp PRIVATE ${COREFOUNDATION_LIBRARY})
  set_target_properties(cvp PROPERTIES
    BUNDLE TRUE
    OUTPUT_NAME "ircamcrepe"
    PREFIX ""
    BUNDLE_EXTENSION "vamp"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resource/Info.plist"
    BUILD_WITH_INSTALL_RPATH FALSE
    LINK_FLAGS "-Wl,-rpath,@loader_path/../Frameworks"
  )
  add_custom_command(TARGET cvp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "BNDL????" > "$<TARGET_BUNDLE_CONTENT_DIR:cvp>/PkgInfo"
  )
elseif(UNIX)
  set_target_properties(cvp PROPERTIES
    BUNDLE TRUE
    OUTPUT_NAME "ircamcrepe"
    PREFIX ""
    BUNDLE_EXTENSION "vamp"
    BUILD_WITH_INSTALL_RPATH FALSE
  )
  add_custom_command(TARGET cvp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/x86_64-linux"
    COMMAND ${CMAKE_COMMAND} -E copy
      "$<TARGET_FILE:cvp>"
      "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/x86_64-linux"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/resource/ircamcrepe.cat"
      "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/Resources"
  )
elseif(WIN32)
  set_target_properties(cvp PROPERTIES
    BUNDLE TRUE
    OUTPUT_NAME "ircamcrepe"
    PREFIX ""
    BUNDLE_EXTENSION "vamp"
  )
  add_custom_command(TARGET cvp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/x86_64-win"
    COMMAND ${CMAKE_COMMAND} -E copy
      "$<TARGET_FILE:cvp>"
      "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/x86_64-win"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_SOURCE_DIR}/resource/ircamcrepe.cat"
      "$<TARGET_FILE_DIR:cvp>/ircamcrepe.vamp/Contents/Resources"
  )
endif()

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
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-Linux.zip](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-Linux.zip)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-MacOS.pkg](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-MacOS.pkg)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-Windows.exe](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-Windows.exe)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crepe-release.md "- [Crepe-Manual.zip](${PROJECT_URL}/releases/download/${CVP_BUILD_TAG}/Crepe-Manual.zip)\n")
endif() 