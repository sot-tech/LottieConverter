cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(lottieconverter)
include(ExternalProject)
include(FindPackageMessage)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(ZLIB_MINIMUM 1.2.3)
set(PNG_MINIMUM 1.6.0)

set(COMPILE_PARAMS -Wall)
set(SOURCE_FILES lottie_export.cpp)
set(LIBS z)
set(INCLUDES)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (CMAKE_BUILD_TYPE EQUAL "RELEASE")
    set(COMPILE_PARAMS ${COMPILE_PARAMS} -O2)
else(CMAKE_BUILD_TYPE)
    set(COMPILE_PARAMS ${COMPILE_PARAMS} -O0 -g3)
endif()

option(SYSTEM_PNG "Use system dynamic libpng" 1)
option(SYSTEM_RL "Use system dynamic rlottie" 0)
option(SYSTEM_GL "Use system shared giflib" 0)

find_package(ZLIB ${ZLIB_MINIMUM} REQUIRED)

if(SYSTEM_PNG)
    find_package(PNG ${PNG_MINIMUM} REQUIRED)
    set(LIBS ${LIBS} png)
else()
    message(NOTICE "-- Building static libpng")
    ExternalProject_Add(
            png
            URL "${CMAKE_SOURCE_DIR}/lib/libpng"
            ${ARGN}
            PREFIX "${CMAKE_BINARY_DIR}/lib/libpng"
            CMAKE_ARGS
            -Wno-dev
            "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
            "-DCMAKE_USER_MAKE_RULES_OVERRIDE=${CMAKE_USER_MAKE_RULES_OVERRIDE}"
            "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
            "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
            "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
            "-DBUILD_SHARED_LIBS=OFF"
            "-DPNG_SHARED=OFF"
            "-DPNG_TESTS=OFF"
            "-DCMAKE_INSTALL_LIBDIR=${CMAKE_BINARY_DIR}/lib/libpng"
            BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/lib/libpng/libpng.a"
    )
    set(INCLUDES ${INCLUDES} "${CMAKE_BINARY_DIR}/lib/libpng/include")
    set(LIBS ${LIBS} pthread "${CMAKE_BINARY_DIR}/lib/libpng/libpng.a")
    add_dependencies(${PROJECT_NAME} png)
endif()

if (SYSTEM_RL)
    find_path(RL_INCLUDEDIR NAMES rlottie.h HINTS ${_RL_INCLUDEDIR} REQUIRED)
    find_library(RL_LIBDIR NAMES rlottie HINTS ${_RL_LIBDIR} REQUIRED)
    find_package_message(rlottie "Found RLOTTIE: ${RL_LIBDIR}" "_")
    set(LIBS ${LIBS} "${RL_LIBDIR}")
else ()
    message(NOTICE "-- Building static rlottie")
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
            BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/lib/rlottie/librlottie.a"
    )
    set(INCLUDES ${INCLUDES} "${CMAKE_BINARY_DIR}/lib/rlottie/include")
    set(LIBS ${LIBS} pthread "${CMAKE_BINARY_DIR}/lib/rlottie/librlottie.a")
    add_dependencies(${PROJECT_NAME} rlottie)
endif ()

if(SYSTEM_GL)
    find_path(GL_INCLUDEDIR NAMES gif_lib.h HINTS ${_GL_INCLUDEDIR} REQUIRED)
    find_library(GL_LIBDIR NAMES gif HINTS ${_GL_LIBDIR} REQUIRED)
    find_package_message(gif "Found GIF: ${GL_LIBDIR}" "_")
    set(LIBS ${LIBS} "${GL_LIBDIR}")
else()
    message(NOTICE "-- Building static giflib")
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
    set(INCLUDES "${INCLUDES}" "${CMAKE_BINARY_DIR}/lib/giflib")
endif()

if(INCLUDES)
    include_directories(${INCLUDES})
endif()

target_compile_options(${PROJECT_NAME} PUBLIC PRIVATE -std=c++${CMAKE_CXX_STANDARD} ${COMPILE_PARAMS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})
