cmake_minimum_required( VERSION 2.7 )
project( SafeLists CXX )

find_package(PkgConfig REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

pkg_check_modules(GTKMM gtkmm-3.0)

SET( PLUMBING_DIR submodules/LibLuaPlumbing/LuaPlumbing )
SET( RAPID_JSON_DIR submodules/rapidjson )
SET( RAPID_JSON_INCLUDE_DIR ${RAPID_JSON_DIR}/include )
SET( SRC_DIR src )
SET( LIBSCRYPT_BUILD_DIR ${CMAKE_BINARY_DIR}/submodules/libscrypt )

ADD_DEFINITIONS(
    -x c++
    -std=c++11
    -g
    -DSAFELISTS_TESTING
    #-O3
)

SET( LIBINCLUDE_DIRS
    ${GTKMM_INCLUDE_DIRS}
    $ENV{TEMPLATIOUS_DIR}
    ${PLUMBING_DIR}
    ${Boost_INCLUDE_DIRS}
    ${RAPID_JSON_INCLUDE_DIR}
    ${LIBSCRYPT_BUILD_DIR} )

SET( APPINCLUDE_DIRS
    ${SRC_DIR}
    ${SRC_DIR}/rust/include )

SET( GTK_SOURCES
    src/gtkmm/GtkMMRangerModel.cpp
    src/gtkmm/GtkMMSessionWidget.cpp
    src/gtkmm/GtkMMFileString.cpp
    src/gtkmm/GenericGtkWidget.cpp
    )

SET( DOMAIN_SOURCES
    src/model/AsyncSqlite.cpp
    src/model/TableSnapshot.cpp
    src/model/SqliteRanger.cpp
    src/util/AutoReg.cpp
    src/util/Misc.cpp
    src/util/GracefulShutdownGuard.cpp
    src/util/GenericStMessageable.cpp
    src/util/Base64.cpp
    src/util/ProgramArgs.cpp
    src/io/RandomFileWriterImpl.cpp
    src/io/Interval.cpp
    src/io/AsyncDownloader.cpp
    src/io/SafeListDownloader.cpp
    src/io/SafeListDownloaderFactory.cpp
    src/io/RandomFileWriter.cpp
    src/model/AsyncSqliteFactory.cpp
    src/meta/Licensing.cpp
    src/meta/SignatureCheck.cpp
    )

include_directories(
    ${LIBINCLUDE_DIRS}
    ${APPINCLUDE_DIRS}
)

SET( RUST_LINK_DIRS ${CMAKE_BINARY_DIR}/rustlib/debug ${CMAKE_BINARY_DIR}/rustlib/release )

link_directories(${GTKMM_LIBRARY_DIRS} ${RUST_LINK_DIRS})

if (UNIX)
    # for newer version of sqlite built and installed
    # from source, ubuntu 14.04 only has sqlite3 3.8.2
    # or something
    link_directories( /usr/local/lib )
endif(UNIX)

add_subdirectory( ${PLUMBING_DIR} )
add_subdirectory( ${RAPID_JSON_DIR} )

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
add_custom_target( copy-root-data ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/rootData/ ${CMAKE_BINARY_DIR} )
add_custom_target( build-safe-dl ALL
    COMMAND CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/rustlib cargo build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/rust/safe_file_downloader )

# <<<<<<< copied code for compilation

add_custom_target( copy-libscrypt-submodule ALL
    COMMAND rsync -avz "${CMAKE_SOURCE_DIR}/submodules/libscrypt" "${CMAKE_BINARY_DIR}/submodules/" )
add_custom_target( build-libscrypt-submodule ALL
    COMMAND make
    DEPENDS copy-libscrypt-submodule
    WORKING_DIRECTORY ${LIBSCRYPT_BUILD_DIR} )
add_library( scrypt STATIC IMPORTED )
add_dependencies( scrypt build-libscrypt-submodule )
set_property( TARGET scrypt PROPERTY IMPORTED_LOCATION ${LIBSCRYPT_BUILD_DIR}/libscrypt.a )

# >>>>>>> copied code for compilation

add_library( domain ${DOMAIN_SOURCES} )
target_link_libraries( domain
    sqlite3 pthread crypto
    curl sodium scrypt plumbing
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    safe_ffi
)

add_dependencies( domain build-safe-dl )

SET( TEST_SOURCES
    tests/model-tests.cpp
    tests/io-tests.cpp
    tests/interval-tests.cpp
    tests/downloader-tests.cpp
    tests/util-tests.cpp
    tests/graceful-shutdown-tests.cpp
    tests/meta-tests.cpp
)

SET( DUMB_HASH_SOURCES
    src/dumbhash.cpp
)

SET( SIGN_JSON_SOURCES
    src/sign-json.cpp
)

add_executable( all-tests ${TEST_SOURCES} )
target_link_libraries( all-tests domain )

add_executable( dumbhash256 ${DUMB_HASH_SOURCES} )
target_link_libraries( dumbhash256 domain )

add_executable( signjson ${SIGN_JSON_SOURCES} )
target_link_libraries( signjson domain )

add_library( gtkspec ${GTK_SOURCES} )
target_link_libraries( gtkspec domain ${GTKMM_LIBRARIES} )

add_executable( safelists src/main.cpp )
target_link_libraries( safelists gtkspec )

