cmake_minimum_required(VERSION 3.27)

project(libmonome VERSION 1.4.10 LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 17)

option(BUILD_EXAMPLES "build libmonome c examples")
option(BUILD_PYTHON_EXTENSION "build cython-based python extension")
option(BUILD_CPP_WRAPPER "build libmonome C++17 wrapper" ON)
option(BUILD_CPP_EXAMPLES "build libmonome C++ examples" OFF)

if(BUILD_CPP_EXAMPLES AND NOT BUILD_CPP_WRAPPER)
    message(FATAL_ERROR "BUILD_CPP_EXAMPLES requires BUILD_CPP_WRAPPER")
endif()

if(BUILD_CPP_WRAPPER)
    enable_language(CXX)
endif()

include(GNUInstallDirs)

if (MSVC)
    add_compile_options(/W4)
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Werror)
elseif (CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-Wall -Werror -Wno-initializer-overrides)
endif()

set(libmonome_sources
    src/libmonome.c
    src/monobright.c
    src/rotation.c
    src/proto/40h.c
    src/proto/mext.c
    src/proto/series.c
    src/platform/embed.c)

set(libmonome_libs)

if(LINUX)
    include(FindPkgConfig)
    pkg_check_modules(libudev REQUIRED IMPORTED_TARGET libudev)

    list(APPEND libmonome_sources
        src/platform/linux_libudev.c
        src/platform/linux.c
        src/platform/posix.c)
    list(APPEND libmonome_libs PkgConfig::libudev)
endif()

if(APPLE)
    list(APPEND libmonome_sources
        src/platform/darwin.c
        src/platform/posix.c)
endif()

if(WIN32)
    list(APPEND libmonome_sources src/platform/windows.c)
    list(APPEND libmonome_libs setupapi)
endif()

add_library(monome_static STATIC ${libmonome_sources})
set_target_properties(monome_static PROPERTIES OUTPUT_NAME "monome")
target_compile_definitions(monome_static PRIVATE EMBED_PROTOS)
target_include_directories(monome_static PRIVATE src/private)
target_include_directories(monome_static PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/public>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(monome_static PRIVATE ${libmonome_libs})

add_library(monome SHARED ${libmonome_sources})
target_compile_definitions(monome PRIVATE EMBED_PROTOS)
target_include_directories(monome PRIVATE src/private)
target_include_directories(monome PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/public>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(monome PRIVATE ${libmonome_libs})

if(BUILD_CPP_WRAPPER)
    add_library(monomepp src/cpp/Monome.cpp)
    target_compile_features(monomepp PUBLIC cxx_std_17)
    target_include_directories(monomepp PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(monomepp PUBLIC monome)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_CPP_EXAMPLES)
    add_executable(simple_toggle examples/cpp/simple_toggle.cpp)
    target_link_libraries(simple_toggle PRIVATE monomepp)
endif()

if(BUILD_TESTING)
    add_library(monome_fake STATIC
        src/libmonome.c
        src/monobright.c
        src/rotation.c
        src/proto/40h.c
        src/proto/mext.c
        src/proto/series.c
        src/platform/embed.c
        tests/fake_platform.c)
    target_compile_definitions(monome_fake PRIVATE
        EMBED_PROTOS
        MONOME_TEST_FAKE_PLATFORM)
    target_include_directories(monome_fake
        PRIVATE
            src/private
            tests
        PUBLIC
            public)

    add_executable(monome_acquisition_tests tests/monome_acquisition_tests.c)
    target_include_directories(monome_acquisition_tests PRIVATE tests)
    target_link_libraries(monome_acquisition_tests PRIVATE monome_fake)
    add_test(NAME monome_acquisition_tests COMMAND monome_acquisition_tests)

    if(BUILD_CPP_WRAPPER)
        add_executable(monome_wrapper_tests
            tests/monome_wrapper_tests.cpp
            src/cpp/Monome.cpp)
        target_compile_features(monome_wrapper_tests PRIVATE cxx_std_17)
        target_include_directories(monome_wrapper_tests PRIVATE include tests)
        target_link_libraries(monome_wrapper_tests PRIVATE monome_fake)
        add_test(NAME monome_wrapper_tests COMMAND monome_wrapper_tests)
    endif()
endif()

if(BUILD_PYTHON_EXTENSION)
    add_subdirectory(bindings/python)
endif()

set(libmonome_install_targets monome_static monome)
if(BUILD_CPP_WRAPPER)
    list(APPEND libmonome_install_targets monomepp)
endif()

install(TARGETS ${libmonome_install_targets} EXPORT libmonomeConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(EXPORT libmonomeConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmonome NAMESPACE libmonome::)
install(DIRECTORY public/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(BUILD_CPP_WRAPPER)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
