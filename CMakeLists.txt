cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(lottieconverter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(ZLIB_MINIMUM 1.2.3)
set(PNG_MINIMUM 1.6.0)
set(RL_MINIMUM 0.2)

set(COMPILE_PARAMS -Wall)
set(SOURCE_FILES lottie_export.cpp)
set(LIBS png z)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (CMAKE_BUILD_TYPE EQUAL "RELEASE")
    set(COMPILE_PARAMS ${COMPILE_PARAMS} -O2)
else(CMAKE_BUILD_TYPE)
    set(COMPILE_PARAMS ${COMPILE_PARAMS} -O0 -g3)
endif()

option(SYSTEM_RL "Use system dynamic rlottie library instead of building static one" 0)

find_package(ZLIB ${ZLIB_MINIMUM} REQUIRED)
find_package(PNG ${PNG_MINIMUM} REQUIRED)

if (SYSTEM_RL)
    find_path(RL_INCLUDEDIR NAMES rlottie.h HINTS ${_RL_INCLUDEDIR} REQUIRED)
    find_library(RL_LIBDIR NAMES rlottie HINTS ${_RL_LIBDIR} REQUIRED)
    set(LIBS ${LIBS} rlottie)
else (SYSTEM_RL)
    include(ExternalProject)
    ExternalProject_Add(
            rlottie
            URL "${CMAKE_SOURCE_DIR}/lib/rlottie"
            ${ARGN}
            PREFIX "${CMAKE_BINARY_DIR}/lib/rlottie"
            CMAKE_ARGS
            -Wno-dev
            "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
            "-DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}"
            "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
            "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
            "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
            "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
            "-DLOTTIE_MODULE=OFF"
            "-DBUILD_SHARED_LIBS=OFF"
            "-DLIB_INSTALL_DIR=${CMAKE_BINARY_DIR}/lib/rlottie"
            BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/librlottie.a"
    )
    ExternalProject_Get_Property(rlottie INSTALL_DIR)
    set(RL_INCLUDEDIR "${INSTALL_DIR}/include")
    set(LIBS ${LIBS} pthread "${INSTALL_DIR}/librlottie.a")
    add_dependencies(${PROJECT_NAME} rlottie)
endif (SYSTEM_RL)

file(COPY ${CMAKE_SOURCE_DIR}/lib/giflib DESTINATION ${CMAKE_BINARY_DIR}/lib)

add_custom_target(
        giflibProject
        COMMAND make libgif.a libutil.a
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/lib/giflib"
        BYPRODUCTS "${CMAKE_BINARY_DIR}/lib/giflib/libgif.a" "${CMAKE_BINARY_DIR}/lib/giflib/libutil.a"
)

add_library(giflibUtil STATIC IMPORTED)
add_dependencies(giflibUtil giflibProject)
set_target_properties(giflibUtil
        PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/giflib/libutil.a"
        )

add_library(giflibMain STATIC IMPORTED)
add_dependencies(giflibMain giflibProject)
set_target_properties(giflibMain
        PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/giflib/libgif.a"
)

add_library(giflib INTERFACE IMPORTED)
set_property(TARGET giflib PROPERTY
        INTERFACE_LINK_LIBRARIES giflibUtil giflibMain)
#add_dependencies(giflib giflibMain giflibUtil)

set(LIBS ${LIBS} giflib)
add_dependencies(${PROJECT_NAME} giflib)

include_directories("${CMAKE_BINARY_DIR}/lib/giflib" "${RL_INCLUDEDIR}")

target_compile_options(${PROJECT_NAME} PUBLIC PRIVATE -std=c++${CMAKE_CXX_STANDARD} ${COMPILE_PARAMS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})
