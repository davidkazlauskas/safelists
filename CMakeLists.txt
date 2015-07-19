cmake_minimum_required( VERSION 2.7 )
project( SafeLists CXX )

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM gtkmm-3.0)

SET( PLUMBING_DIR submodules/LibLuaPlumbing/LuaPlumbing )

include_directories(${GTKMM_INCLUDE_DIRS} $ENV{TEMPLATIOUS_DIR} ${PLUMBING_DIR} )
link_directories(${GTKMM_LIBRARY_DIRS})

add_subdirectory( ${PLUMBING_DIR} )
