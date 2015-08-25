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
    src/util/AutoReg.cpp
    src/io/RandomFileWriter.cpp
    src/io/Interval.cpp
    )

include_directories(
    ${LIBINCLUDE_DIRS}
    ${APPINCLUDE_DIRS}
)

link_directories(${GTKMM_LIBRARY_DIRS})
if (UNIX)
    # for newer version of sqlite built and installed
    # from source, ubuntu 14.04 only has sqlite3 3.8.2
    # or something
    link_directories( /usr/local/lib )
endif(UNIX)

add_subdirectory( ${PLUMBING_DIR} )

SET( CMAKE_EXPORT_COMPILE_COMMANDS yes )

add_custom_target( copy-runtime-files ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/src/uischemes/ ${CMAKE_BINARY_DIR}/uischemes/ )
add_custom_target( copy-runtime-files-ex-data ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/exampleData/ ${CMAKE_BINARY_DIR}/exampleData/ )
add_custom_target( copy-lua-plumbing ALL
    COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/${PLUMBING_DIR}/plumbing.lua ${CMAKE_BINARY_DIR}/lua/plumbing.lua )
add_custom_target( copy-lua-debug ALL
    COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/${PLUMBING_DIR}/mobdebug.lua ${CMAKE_BINARY_DIR}/lua/mobdebug.lua )
add_custom_target( copy-lua-scripts ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/src/lua ${CMAKE_BINARY_DIR}/lua )

add_library( domain ${DOMAIN_SOURCES} )
target_link_libraries( domain sqlite3 pthread )

SET( TEST_SOURCES
    tests/model-tests.cpp
    tests/io-tests.cpp )

add_executable( all-tests ${TEST_SOURCES} )
target_link_libraries( all-tests domain )

add_library( gtkspec ${GTK_SOURCES} )
target_link_libraries( gtkspec domain ${GTKMM_LIBRARIES} )

add_executable( safelists src/main.cpp )
target_link_libraries( safelists gtkspec plumbing )

