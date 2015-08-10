cmake_minimum_required( VERSION 2.7 )
project( SafeLists CXX )

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM gtkmm-3.0)

SET( PLUMBING_DIR submodules/LibLuaPlumbing/LuaPlumbing )
SET( SRC_DIR src )

ADD_DEFINITIONS(
    -x c++
    -std=c++11
    -g
)

SET( LIBINCLUDE_DIRS
    ${GTKMM_INCLUDE_DIRS}
    $ENV{TEMPLATIOUS_DIR}
    ${PLUMBING_DIR} )

SET( APPINCLUDE_DIRS
    ${SRC_DIR} )

SET( GTK_SOURCES
    src/gtkmm/GtkMMRangerModel.cpp
    )

SET( DOMAIN_SOURCES
    src/model/AsyncSqlite.cpp
    src/model/TableSnapshot.cpp
    src/model/SqliteRanger.cpp
    )

include_directories(
    ${LIBINCLUDE_DIRS}
    ${APPINCLUDE_DIRS}
)

link_directories(${GTKMM_LIBRARY_DIRS})

add_subdirectory( ${PLUMBING_DIR} )

SET( CMAKE_EXPORT_COMPILE_COMMANDS yes )

add_custom_target( copy-runtime-files ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/src/uischemes/ ${CMAKE_BINARY_DIR}/uischemes/ )

add_library( gtkspec ${GTK_SOURCES} )
target_link_libraries( gtkspec ${GTKMM_LIBRARIES} )

add_library( domain ${DOMAIN_SOURCES} )
target_link_libraries( domain sqlite3 pthread )

add_executable( model-tests tests/model-tests.cpp )
target_link_libraries( model-tests domain )

add_executable( safelists src/main.cpp )
target_link_libraries( safelists domain gtkspec )

