##################################################    Project     ##################################################
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project               (lite-obs VERSION 1.0 LANGUAGES CXX C)
list                  (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set_property          (GLOBAL PROPERTY USE_FOLDERS ON)

option(LITEOBS_WITH_SANITIZER "Use [address|thread|undefined] here" OFF)
option(LITEOBS_WITH_RTTI "Compile with runtime type information" OFF)
option(LITEOBS_WITH_WERROR "Make all compilation warnings errors" ON)

add_library(
    liteobs-compiler-options INTERFACE
)

set(UBSAN_BLACKLIST "")
target_compile_options(
    liteobs-compiler-options
    INTERFACE
    $<$<BOOL:${LITEOBS_WITH_SANITIZER}>:-DSANITIZE>
    $<$<BOOL:${LITEOBS_WITH_SANITIZER}>:-fno-omit-frame-pointer>
    $<$<BOOL:${LITEOBS_WITH_SANITIZER}>:-fno-optimize-sibling-calls>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},address>:-fsanitize=address>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},thread>:-fsanitize=thread>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},undefined>:-fsanitize=implicit-conversion,
    -fsanitize-blacklist=${UBSAN_BLACKLIST}>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},undefined>:-fsanitize=undefined,
    -fsanitize-blacklist=${UBSAN_BLACKLIST}>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},undefined>:-fsanitize=unsigned-integer-overflow,
    -fsanitize-blacklist=${UBSAN_BLACKLIST}>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},undefined>:-fsanitize=nullability,
    -fsanitize-blacklist=${UBSAN_BLACKLIST}>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},undefined>:-fsanitize=float-divide-by-zero,
    -fsanitize-blacklist=${UBSAN_BLACKLIST}>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<BOOL:${LITEOBS_WITH_RTTI}>>,$<NOT:$<PLATFORM_ID:Windows>>>:-fno-rtti>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<PLATFORM_ID:Windows>>>:-Wall>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<PLATFORM_ID:Windows>>>:-Wshadow>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<PLATFORM_ID:Windows>>>:-Wextra>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<PLATFORM_ID:Windows>>,$<BOOL:${LITEOBS_WITH_WERROR}>>:-Werror>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<PLATFORM_ID:Android>>:-Wno-error=tautological-constant-compare>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=maybe-uninitialized>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=return-type>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=unknown-pragmas>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=deprecated-volatile>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-volatile>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-error=deprecated-declarations>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-error=unused-parameter>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-error=unused-property-ivar>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-error=deprecated-volatile>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-unused-parameter>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-deprecated-declarations>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-deprecated-volatile>
)

target_link_libraries(
    liteobs-compiler-options
    INTERFACE
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},address>:-fsanitize=address>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},thread>:-fsanitize=thread>
    $<$<STREQUAL:${LITEOBS_WITH_SANITIZER},undefined>:-fsanitize=undefined>
)

cmake_policy(SET CMP0063 NEW)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

##################################################    Options     ##################################################
option(BUILD_EXAMPLES "Build examples." OFF)

##################################################    Sources     ##################################################
unset(PROJECT_SOURCES CACHE)
file(GLOB_RECURSE PROJECT_HEADERS include/*.h include/*.hpp include/*.ipp)
file(GLOB_RECURSE PROJECT_SOURCES source/*.cpp source/*.mm source/*.c source/*.h)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_SOURCE_DIR}/source/third-party/.*$")
file(GLOB_RECURSE PROJECT_CMAKE_UTILS cmake/*.cmake)
file(GLOB_RECURSE PROJECT_MISC *.bat *.gitignore *.md *.py *.sh *.txt)

set (PROJECT_FILES
    ${PROJECT_HEADERS}
    ${PROJECT_SOURCES}
    ${PROJECT_PLATFORM_HEADERS}
    ${PROJECT_PLATFORM_SOURCES}
    ${PROJECT_CMAKE_UTILS}
    ${PROJECT_MISC})

include            (assign_source_group)
assign_source_group(${PROJECT_FILES})

if(ANDROID OR APPLE)
    #library search issue. https://stackoverflow.com/questions/46055431/cmake-on-android-find-library-fail-to-find-with-custom-path
    #for cross compile
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()

add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE -DBUILD_LITE_OBS_LIB)

##################################################    Depends     ##################################################
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL Android)
    include(${PROJECT_SOURCE_DIR}/cmake/platform/android.cmake)
elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
    include(${PROJECT_SOURCE_DIR}/cmake/platform/windows.cmake)
else()
    if(IOS OR CMAKE_SYSTEM_NAME STREQUAL iOS)
        include(${PROJECT_SOURCE_DIR}/cmake/platform/ios.cmake)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        include(${PROJECT_SOURCE_DIR}/cmake/platform/macos.cmake)
    else()
        message(FATAL_ERROR "Unsupported target platform: " ${CMAKE_SYSTEM_NAME})
    endif()
endif()

set(FFMPEG_PATH ${PROJECT_SOURCE_DIR}/external/${TARGET_OS}/ffmpeg)
set(THIRDPARTY_PATH ${PROJECT_SOURCE_DIR}/external/${TARGET_OS}/thirdparty)

include(import_library)

find_package(FFmpeg REQUIRED)
import_library(FFmpeg_INCLUDE_DIRS FFmpeg_LIBS)

find_package(SRT REQUIRED)
import_library(SRT_INCLUDE_DIRS SRT_LIBS)

find_package(x264 REQUIRED)
import_library(SRT_INCLUDE_DIRS X264_LIB)

#define NO_CRYPTO to disable rtmps. openssl exists because srt depends on it, so define HAVE_OPENSSL
target_compile_definitions(${PROJECT_NAME} PUBLIC NO_CRYPTO HAVE_OPENSSL)

add_subdirectory(source/third-party/glm)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include> PRIVATE source)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries     (${PROJECT_NAME} PUBLIC ${PROJECT_LIBRARIES} liteobs-compiler-options glm)

##################################################    install     ##################################################
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/output/${TARGET_OS}"
        CACHE PATH "default install path" FORCE)
endif()

if(ANDROID_ABI)
    set(OUTPUT_ARCH ${ANDROID_ABI})
elseif(CMAKE_ANDROID_ARCH_ABI)
    set(OUTPUT_ARCH ${CMAKE_ANDROID_ARCH_ABI})
elseif(CMAKE_C_COMPILER_ARCHITECTURE_ID) # msvc
    set(OUTPUT_ARCH ${CMAKE_C_COMPILER_ARCHITECTURE_ID}) # ARMV7 ARM64 X86 x64
elseif(WIN32)
    set(OUTPUT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "[aA].*[rR].*64")
        set(OUTPUT_ARCH arm64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "[aA][rR][mM]")
        set(OUTPUT_ARCH armhf)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "64")
        set(OUTPUT_ARCH amd64)
    endif()
endif()
string(TOLOWER "${OUTPUT_ARCH}" OUTPUT_ARCH)
if(WIN32)
    if(OUTPUT_ARCH MATCHES armv7) #msvc
        set(OUTPUT_ARCH arm)
    elseif(OUTPUT_ARCH MATCHES amd64) #msvc
        set(OUTPUT_ARCH x64)
    endif()
endif()

if (${TARGET_OS} STREQUAL iOS)
    install(TARGETS ${PROJECT_NAME}
        FRAMEWORK DESTINATION .)
else()
    set(EXPORT_HEADERS
        ${PROJECT_SOURCE_DIR}/include/lite-obs/lite_obs.h
        ${PROJECT_SOURCE_DIR}/include/lite-obs/lite_obs_callback.h
        ${PROJECT_SOURCE_DIR}/include/lite-obs/lite_obs_global.h
        ${PROJECT_SOURCE_DIR}/include/lite-obs/lite_obs_defines.h
    )

    install(FILES ${EXPORT_HEADERS} DESTINATION include/lite-obs)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lib/FindLiteOBS.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

    set(SDK_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR}/${OUTPUT_ARCH})
    set(SDK_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${OUTPUT_ARCH})

    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${SDK_INSTALL_BINDIR} # *.dll
        LIBRARY DESTINATION ${SDK_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${SDK_INSTALL_LIBDIR})
    if (FFmpeg_BIN_FILE)
        if(${TARGET_OS} STREQUAL android)
            install(FILES ${FFmpeg_BIN_FILE} DESTINATION ${SDK_INSTALL_LIBDIR})
        elseif(${TARGET_OS} STREQUAL windows-desktop)
            install(FILES ${FFmpeg_BIN_FILE} DESTINATION ${SDK_INSTALL_BINDIR})
        endif()
    endif()
endif()

##################################################    Examples    ##################################################
if(BUILD_EXAMPLES)
    add_subdirectory(examples/qt)
endif()
