cmake_minimum_required(VERSION 3.10)

# This "outer" CMake file works as a dependency fetcher, and it consists almost exclusively of ExternalProject commands.
# Meanwhile, the "inner" CMake file under project/ is a more traditional CMake file. It uses the find_* commands to
# locate dependencies without any knowledge or assumptions about where those dependencies live or how they got there.
# This separation into two CMake files was necessary because the find_* commands run when we configure the project, but
# the ExternalProject commands don't run until we build the project. By splitting the process into two CMake files, I
# can ensure the ExternalProject commands run first, then configure (run cmake on) the real project only after the
# dependencies are ready.

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(ENABLE_TESTING "Whether to build the test and bench harness and perform testing." FALSE)
message(STATUS "Enable testing: ${ENABLE_TESTING}")

include(ExternalProject)

# Setting EP_BASE gets us a better directory structure than the legacy default
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects")

ExternalProject_Add(
    boost
    URL https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.gz
    EXCLUDE_FROM_ALL TRUE
    BUILD_IN_SOURCE TRUE
    # If testing is enabled, then we need to configure and build several boost libraries, but if
    # testing is not enabled, then we only depend on the header files
    CONFIGURE_COMMAND $<$<BOOL:${ENABLE_TESTING}>:$<IF:$<BOOL:${CMAKE_HOST_UNIX}>,./bootstrap.sh,bootstrap.bat>>
    # Conditional build command will come later; it had to be defined in a custom step so that
    # special tokens such as <SOURCE_DIR> would work
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
if(ENABLE_TESTING)
    ExternalProject_Add_Step(
        boost build_testing
        DEPENDEES build DEPENDERS install
        WORKING_DIRECTORY "<SOURCE_DIR>"

        COMMAND
            # An out-of-source install would be ideal, but that involves copying
            # all the header files, which is super slow on windows
            ./b2 "--stagedir=<SOURCE_DIR>"
            --with-filesystem --with-program_options --with-test
            variant=$<IF:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>,debug,release>
            # Boost doesn't seem to infer the appropriate address model from
            # Visual Studio's platform name / architecture
            $<$<STREQUAL:${CMAKE_GENERATOR_PLATFORM},x86>:address-model=32> $<$<STREQUAL:${CMAKE_GENERATOR_PLATFORM},x64>:address-model=64>
            link=static stage
    )
endif()
ExternalProject_Get_Property(boost SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

find_program(PYTHON_COMMAND python)
if(PYTHON_COMMAND STREQUAL "PYTHON_COMMAND-NOTFOUND")
    message(FATAL_ERROR "Check for python: PYTHON COMMAND NOT FOUND")
else()
    message(STATUS "Check for python: ${PYTHON_COMMAND}")
endif()
ExternalProject_Add(
    python_patch
    URL https://github.com/techtonik/python-patch/archive/1.16.tar.gz
    EXCLUDE_FROM_ALL TRUE
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
)
ExternalProject_Get_Property(python_patch SOURCE_DIR)
set(python_patch_SOURCE_DIR "${SOURCE_DIR}")

ExternalProject_Add(
    gsl
    URL https://github.com/Microsoft/GSL/archive/1f87ef73f1477e8adafa8b10ccee042897612a20.zip
    DEPENDS python_patch
    EXCLUDE_FROM_ALL TRUE
    PATCH_COMMAND "${python_patch_SOURCE_DIR}/patch.py" "--directory=<SOURCE_DIR>"
        "${CMAKE_CURRENT_SOURCE_DIR}/patch/gsl/final_action_move_assign.diff"
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gsl SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

ExternalProject_Add(
    gcpp
    DEPENDS gsl python_patch
    URL https://github.com/hsutter/gcpp/archive/3be430dd62fe29e92b42c66fd1f9ee3803edbca4.zip
    EXCLUDE_FROM_ALL TRUE
    PATCH_COMMAND "${python_patch_SOURCE_DIR}/patch.py" "--directory=<SOURCE_DIR>"
        "${CMAKE_CURRENT_SOURCE_DIR}/patch/gcpp/static_pointer_cast.diff"
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gcpp SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

ExternalProject_Add(
    google_benchmark
    URL https://github.com/google/benchmark/archive/v1.3.0.tar.gz
    EXCLUDE_FROM_ALL TRUE
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DBENCHMARK_ENABLE_TESTING=FALSE
        "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
)
ExternalProject_Get_Property(google_benchmark INSTALL_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${INSTALL_DIR}")

ExternalProject_Add(
    craftinginterpreters
    URL https://github.com/munificent/craftinginterpreters/archive/d2800ebca6ab6d56f698ad0eef8202a91821eb04.zip
    EXCLUDE_FROM_ALL TRUE
    # CraftingInterpreters uses non-portable makefiles; give it a CMake script instead
    PATCH_COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/patch/craftinginterpreters/jlox_build.cmake" "<SOURCE_DIR>/jlox_build.cmake"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/patch/craftinginterpreters/jlox_run.cmake" "<SOURCE_DIR>/jlox_run.cmake"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "${CMAKE_COMMAND}" -P "<SOURCE_DIR>/jlox_build.cmake"
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(craftinginterpreters SOURCE_DIR)
set(INSTALLATION_PREFIXES "${INSTALLATION_PREFIXES}$<SEMICOLON>${SOURCE_DIR}")

if(ENABLE_TESTING)
    find_program(JAVAC_COMMAND javac)
    message(STATUS "Check for javac: ${JAVAC_COMMAND}")
endif()

# A convenience target; it lets us compile dependencies in a separate step from our project
add_custom_target(deps DEPENDS
    boost gsl gcpp
    $<$<BOOL:${ENABLE_TESTING}>:google_benchmark>
    $<$<AND:$<BOOL:${ENABLE_TESTING}>,$<NOT:$<STREQUAL:${JAVAC_COMMAND},JAVAC_COMMAND-NOTFOUND>>>:craftinginterpreters>
)

# Now that we have our dependencies on disk to be found, it's safe to configure (run cmake on) the real project
ExternalProject_Add(
    main
    DEPENDS deps
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/project"
    BUILD_ALWAYS TRUE
    CMAKE_ARGS
        "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_PREFIX_PATH=${INSTALLATION_PREFIXES}"
        "-DENABLE_TESTING=${ENABLE_TESTING}"
        "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/DIST"
    TEST_AFTER_INSTALL "${ENABLE_TESTING}"
    # Override test command so we can specify verbose, otherwise the test harness's output is suppressed
    TEST_COMMAND "${CMAKE_CTEST_COMMAND}" --build-config "${CMAKE_BUILD_TYPE}" --verbose
)
if(ENABLE_TESTING)
    ExternalProject_Add_Step(
        main bench DEPENDEES test
        COMMAND "${CMAKE_COMMAND}" --build "<BINARY_DIR>" --target bench --config "${CMAKE_BUILD_TYPE}"
    )
endif()
