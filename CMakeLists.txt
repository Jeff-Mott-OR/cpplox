cmake_minimum_required(VERSION 3.10)

include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects")

# Boost
if(CMAKE_HOST_UNIX)
    set(BOOST_CONFIGURE_COMMAND ./bootstrap.sh)
else()
    set(BOOST_CONFIGURE_COMMAND bootstrap.bat)
endif()
ExternalProject_Add(
    boost
    URL https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz

    # Don't build unless the main project depends on it
    EXCLUDE_FROM_ALL TRUE

    # Set the working directory to the source dir
    BUILD_IN_SOURCE TRUE

    CONFIGURE_COMMAND "${BOOST_CONFIGURE_COMMAND}"

    # An out-of-source install would be ideal, but that involves copying all the
    # header files, which is super slow on windows
    BUILD_COMMAND ./b2 "--stagedir=<SOURCE_DIR>" --with-filesystem --with-test variant=release link=static stage
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(boost SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

# GSL
ExternalProject_Add(
    gsl
    URL https://github.com/Microsoft/GSL/archive/1f82596e1dada0067712527262a3d561ad51ddac.zip

    # Don't build unless the main project depends on it
    EXCLUDE_FROM_ALL TRUE

    # Header-only library; nothing to configure, build, or install
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gsl SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

# Main
# Now that we have our dependencies on disk to be found,
# it's safe to configure (run cmake on) the real project.
ExternalProject_Add(
    main
    DEPENDS boost gsl

    # The real project is already on disk
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/project"

    CMAKE_ARGS
        # Tell the real project where to find the dependencies
        "-DCMAKE_PREFIX_PATH=${INSTALLATION_PREFIXES}"

        # Also build the test harness
        -DENABLE_TESTING=TRUE

        # Force install into ExternalProjects directory, otherwise it will try to install
        # into global system default location.
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"

    TEST_AFTER_INSTALL TRUE
)
