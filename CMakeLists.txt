cmake_minimum_required( VERSION 2.7 )
project( SafeLists CXX )

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM gtkmm-3.0)

SET( PLUMBING_DIR submodules/LibLuaPlumbing/LuaPlumbing )
SET( SRC_DIR src )

SET( LIBINCLUDE_DIRS
    ${GTKMM_INCLUDE_DIRS}
    $ENV{TEMPLATIOUS_DIR}
    ${PLUMBING_DIR} )

SET( APPINCLUDE_DIRS
    ${SRC_DIR} )

include_directories(
    ${LIBINCLUDE_DIRS}
    ${APPINCLUDE_DIRS}
)

link_directories(${GTKMM_LIBRARY_DIRS})

add_subdirectory( ${PLUMBING_DIR} )

SET( CMAKE_EXPORT_COMPILE_COMMANDS yes )

add_executable( model-tests tests/model-tests.cpp )

