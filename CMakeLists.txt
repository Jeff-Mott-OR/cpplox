cmake_minimum_required(VERSION 3.10)

# This "outer" CMake file works as a dependency fetcher, and it consists almost exclusively of ExternalProject commands.
# Meanwhile, the "inner" CMake file under project/ is a more traditional CMake file. It uses the find_* commands to
# locate dependencies without any knowledge or assumptions about where those dependencies live or how they got there.
# This separation into two CMake files was necessary because the find_* commands run when we configure the project, but
# the ExternalProject commands don't run until we build the project. That order is the opposite of what we need. It
# means CMake will try to find dependencies _before_ it downloads and builds those dependencies. By splitting the
# process into two CMake files, we can ensure the ExternalProject commands run first, then configure (run cmake on) the
# real project only after the dependencies are ready.

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(ENABLE_TESTING "Whether to build the test harness and perform testing." FALSE)
message(STATUS "Enable testing: ${ENABLE_TESTING}")

include(ExternalProject)

# Setting EP_BASE gets us a better directory structure than the legacy default
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects")

# If testing is enabled, then we need to configure and build several boost libraries. But if testing is not enabled,
# then we only depend on the header files.
if(ENABLE_TESTING)
    if(CMAKE_HOST_UNIX)
        set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
    else()
        set(BOOST_CONFIGURE_COMMAND bootstrap.bat)
    endif()
else()
    set(BOOST_CONFIGURE_COMMAND "")
endif()
ExternalProject_Add(
    boost
    URL https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
    EXCLUDE_FROM_ALL TRUE
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND "${BOOST_CONFIGURE_COMMAND}"
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
if(ENABLE_TESTING)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BOOST_BUILD_VARIANT debug)
    else()
        set(BOOST_BUILD_VARIANT release)
    endif()
    ExternalProject_Add_Step(
        boost build_testing
        DEPENDEES build DEPENDERS install
        WORKING_DIRECTORY "<SOURCE_DIR>"

        # An out-of-source install would be ideal, but that involves copying all the header files, which is super slow
        # on windows.
        COMMAND
            ./b2 "--stagedir=<SOURCE_DIR>"
            --with-filesystem --with-program_options --with-test
            "variant=${BOOST_BUILD_VARIANT}" link=static stage
    )
endif()
ExternalProject_Get_Property(boost SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

ExternalProject_Add(
    gsl
    URL https://github.com/Microsoft/GSL/archive/1f82596e1dada0067712527262a3d561ad51ddac.zip
    EXCLUDE_FROM_ALL TRUE

    # Header-only library; nothing to configure, build, or install
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gsl SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

ExternalProject_Add(
    google_benchmark
    URL https://github.com/google/benchmark/archive/v1.3.0.tar.gz
    EXCLUDE_FROM_ALL TRUE
    CMAKE_ARGS
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        -DBENCHMARK_ENABLE_TESTING=FALSE
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
)
ExternalProject_Get_Property(google_benchmark INSTALL_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${INSTALL_DIR}")

# Now that we have our dependencies on disk to be found, it's safe to configure (run cmake on) the real project
ExternalProject_Add(
    main
    DEPENDS boost gsl

    # The main project is already on disk
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/project"

    CMAKE_ARGS
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_PREFIX_PATH=${INSTALLATION_PREFIXES}"
        "-DENABLE_TESTING=${ENABLE_TESTING}"
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
    TEST_AFTER_INSTALL "${ENABLE_TESTING}"

    # Override test command so we can specify verbose, otherwise the test harness's output is suppressed
    TEST_COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "${CMAKE_BUILD_TYPE}" --verbose
)
if(ENABLE_TESTING)
    add_dependencies(main google_benchmark)
    ExternalProject_Add_Step(
        main bench DEPENDEES test
        COMMAND "${CMAKE_COMMAND}" --build "<BINARY_DIR>" --target bench --config "${CMAKE_BUILD_TYPE}"
    )
endif()
