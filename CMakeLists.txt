cmake_minimum_required(VERSION 3.10)

# Build type is meaningful even to multi-configuration generators (such as
# MSVC) for building dependencies.
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Build type not defined. Defaulting to Release.")
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_TESTING "Whether to build the test harness and perform testing." TRUE)

include(ExternalProject)

# Setting EP_BASE gets us a better directory structure than the legacy default
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects")

# Boost
if(CMAKE_HOST_UNIX)
    set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
else()
    set(BOOST_CONFIGURE_COMMAND bootstrap.bat)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BOOST_BUILD_VARIANT debug)
else()
    set(BOOST_BUILD_VARIANT release)
endif()
ExternalProject_Add(
    boost
    URL https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz

    # Don't build unless a project depends on it
    EXCLUDE_FROM_ALL TRUE

    # Set the working directory to the source dir
    BUILD_IN_SOURCE TRUE

    CONFIGURE_COMMAND "${BOOST_CONFIGURE_COMMAND}"

    # An out-of-source install would be ideal, but that involves copying all the
    # header files, which is super slow on windows
    BUILD_COMMAND ./b2 "--stagedir=<SOURCE_DIR>" --with-filesystem --with-program_options --with-test "variant=${BOOST_BUILD_VARIANT}" link=static stage
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(boost SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

# GSL
ExternalProject_Add(
    gsl
    URL https://github.com/Microsoft/GSL/archive/1f82596e1dada0067712527262a3d561ad51ddac.zip

    # Don't build unless a project depends on it
    EXCLUDE_FROM_ALL TRUE

    # Header-only library; nothing to configure, build, or install
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gsl SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

# GTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/release-1.8.0.tar.gz

    # Don't build unless a project depends on it
    EXCLUDE_FROM_ALL TRUE

    CMAKE_ARGS
        # Pass through build type
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"

        # Force install into ExternalProjects directory, otherwise it will try to install
        # into global system default location.
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
)
ExternalProject_Get_Property(gtest INSTALL_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${INSTALL_DIR}")

# Google Benchmark
ExternalProject_Add(
    google_benchmark
    URL https://github.com/google/benchmark/archive/v1.3.0.tar.gz

    # Supposedly Google Benchmark requires GTest, but I haven't observed that yet
    # DEPENDS gtest

    # Don't build unless a project depends on it
    EXCLUDE_FROM_ALL TRUE

    CMAKE_ARGS
        # Pass through build type
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"

        # Tell the project where to find the dependencies
        "-DCMAKE_PREFIX_PATH=${INSTALLATION_PREFIXES}"

        # Force install into ExternalProjects directory, otherwise it will try to install
        # into global system default location.
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
)
ExternalProject_Get_Property(google_benchmark INSTALL_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${INSTALL_DIR}")

# Main
# Now that we have our dependencies on disk to be found, it's safe to configure
# (run cmake on) the real project.
ExternalProject_Add(
    main
    DEPENDS boost gsl google_benchmark

    # The main project is already on disk
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/project"

    CMAKE_ARGS
        # Tell the project where to find the dependencies
        "-DCMAKE_PREFIX_PATH=${INSTALLATION_PREFIXES}"

        # Conditionally also build the test harness
        "-DENABLE_TESTING=${ENABLE_TESTING}"

        # Force install into ExternalProjects directory, otherwise it will try to install
        # into global system default location.
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"

    TEST_AFTER_INSTALL "${ENABLE_TESTING}"

    # Override test command just so we can specify verbose, otherwise the test
    # harness's output would be suppressed
    TEST_COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "${CMAKE_BUILD_TYPE}" --verbose
)
if(ENABLE_TESTING)
    ExternalProject_Add_Step(
        main bench DEPENDEES test
        COMMAND "${CMAKE_COMMAND}" --build "<BINARY_DIR>" --target bench --config "${CMAKE_BUILD_TYPE}"
    )
endif()
