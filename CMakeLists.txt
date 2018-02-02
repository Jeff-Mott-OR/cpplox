cmake_minimum_required(VERSION 3.10)

include(ExternalProject)

# Setting EP_BASE gets us a better directory structure than the legacy default
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects")

# Boost
ExternalProject_Add(
    boost
    URL https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz

    # Don't build unless a project depends on it
    EXCLUDE_FROM_ALL 1

    # Header-only library; nothing to configure, build, or install
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(boost SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

# GSL
ExternalProject_Add(
    gsl
    URL https://github.com/Microsoft/GSL/archive/1f82596e1dada0067712527262a3d561ad51ddac.zip

    # Don't build unless a project depends on it
    EXCLUDE_FROM_ALL 1

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

    # The main project is already on disk
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/project"

    # Tell the project where to find the dependencies
    CMAKE_ARGS "-DCMAKE_PREFIX_PATH=${INSTALLATION_PREFIXES}"

    INSTALL_COMMAND ""
)
