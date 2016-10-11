cmake_minimum_required( VERSION 2.7 )
project( SafeLists CXX )

find_package(PkgConfig REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

# MXE dependencies:
# make sqlite gtkmm3 lua sodium adwaita-icon-theme boost

macro(ensure_pkg_version action package version var)
    pkg_check_modules(${var} ${package})
    if(NOT ${var}_FOUND)
        message(FATAL_ERROR "Package ${package} doesn't exist.")
    endif()
    if(NOT ${var}_VERSION ${action} ${version})
        message(FATAL_ERROR "Version requirement for ${package} not satisfied."
            " (Version expected: |${version}|,Actual: |${${var}_VERSION}|,"
            " Comparison: |${action}|)")
    endif()
endmacro()

SET( SAFELISTS_VERSION "1.0.0" )
SET( SAFELISTS_PRODUCTION_KEY_BOXING  "LtwTrQKKoNBK9cdW3aNizhu/zCJ4NBZo66KtxXCUXg0=" )
SET( SAFELISTS_PRODUCTION_KEY_SIGNING "6D9ezHBmnJOEAfKlt5hTpU1/OLcPWiTvtpgiWH/cqy8=" )

SET( GTK3_VERSION_REQUIRED 3.14.4 )
SET( GTKMM_VERSION_REQUIRED 3.14.0 )
ensure_pkg_version(STREQUAL gtk+-3.0 ${GTK3_VERSION_REQUIRED} GTK3)
ensure_pkg_version(STREQUAL gtkmm-3.0 ${GTKMM_VERSION_REQUIRED} GTKMM)

SET( MKDIR /bin/mkdir )
SET( RSYNC /usr/bin/rsync )
SET( RSYNC_ARGS -rlDz )
SET( FLATGLOBCOPY ${CMAKE_SOURCE_DIR}/buildutil/flatglobcopy )

SET( PLUMBING_DIR submodules/LibLuaPlumbing/LuaPlumbing )
SET( SRC_DIR src )
SET( LIBSCRYPT_DIR ${CMAKE_SOURCE_DIR}/submodules/libscrypt )

ADD_DEFINITIONS(
    -x c++
    -std=c++11
    -g
    -DSAFELISTS_TESTING
    -O3
)

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR USE_PROD_KEYS)
    ADD_DEFINITIONS(
        -DSAFELISTS_KEY_FOR_SIGN="${SAFELISTS_PRODUCTION_KEY_SIGNING}"
    )
    message("Using production public keys for authentication.")
endif(CMAKE_BUILD_TYPE STREQUAL "Release" OR USE_PROD_KEYS)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    ADD_DEFINITIONS(
        -DSAFELISTS_RELEASE_BUILD="true"
    )
endif()

SET( LIBINCLUDE_DIRS
    ${GTKMM_INCLUDE_DIRS}
    $ENV{TEMPLATIOUS_DIR}
    ${PLUMBING_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${RAPID_JSON_INCLUDE_DIR}
    ${LIBSCRYPT_DIR} )

SET( APPINCLUDE_DIRS
    ${SRC_DIR}
    ${SRC_DIR}/rust/include )

SET( GTK_SOURCES
    src/gtkmm/GtkMMRangerModel.cpp
    src/gtkmm/GtkMMSessionWidget.cpp
    src/gtkmm/GtkMMFileString.cpp
    src/gtkmm/GtkMMThemeManager.cpp
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
    src/util/ProgramArgs.cpp
    src/io/RandomFileWriterImpl.cpp
    src/io/Interval.cpp
    src/io/AsyncDownloader.cpp
    src/io/SafeListDownloader.cpp
    src/io/SafeListDownloaderFactory.cpp
    src/io/RandomFileWriter.cpp
    src/model/AsyncSqliteFactory.cpp
    src/meta/GlobalConsts.cpp
    )

include_directories(
    ${LIBINCLUDE_DIRS}
    ${APPINCLUDE_DIRS}
)

SET( RUST_LIB_DIR ${CMAKE_BINARY_DIR}/rustlib )
SET( RUST_LINK_DIR ${CMAKE_BINARY_DIR} )

link_directories(${GTKMM_LIBRARY_DIRS} ${RUST_LINK_DIR})

if (UNIX)
    # for newer version of sqlite built and installed
    # from source, ubuntu 14.04 only has sqlite3 3.8.2
    # or something
    link_directories( /usr/local/lib )
endif(UNIX)

if(EXISTS "${CMAKE_PREFIX_PATH}/include/lua.hpp")
    SET( CMAKE_CXX_FLAGS "-DCURL_STATICLIB -DPLUMBING_LUA_INCLUDE=\"<lua.hpp>\"" )
    if(EXISTS "${CMAKE_PREFIX_PATH}/lib/liblua.a"
        OR EXISTS "${CMAKE_PREFIX_PATH}/lib/liblua.so")
        SET( PLUMBING_LUA_LIBS lua )
    else()
        message(FATAL_ERROR "Found lua header but cannot find library... Cannot continue")
    endif()
endif()

option( RAPID_JSON_BUILD_DOC OFF )
option( RAPID_JSON_BUILD_EXAMPLES OFF )
option( RAPID_JSON_BUILD_TESTS OFF )
option( RAPID_JSON_BUILD_THIRDPARTY_GTEST OFF )
add_subdirectory( ${PLUMBING_DIR} )

SET( CMAKE_EXPORT_COMPILE_COMMANDS yes )

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    SET(CARGO_BUILD_MODE "--release")
else()
    SET(CARGO_BUILD_MODE "")
endif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")

# just guessing, should be more robust in the future
SET( TARGET_ARCHITECTURE "i686" )
SET( CARGO_COMMAND cargo build )
if(CMAKE_CROSS_COMPILING)
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux"
       OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        # support musl in future?
        SET( CARGO_COMMAND PKG_CONFIG_ALLOW_CROSS=1
            CC=${CMAKE_C_COMPILER}
            cargo rustc ${CARGO_BUILD_MODE}
            --target=${TARGET_ARCHITECTURE}-unknown-linux-gnu
            -Clinker=${CMAKE_C_COMPILER} )
    endif()
endif()

SET( ICON_RESOURCE "" )
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    # icon generated with:
    # $MXE_PATH/usr/bin/i686-w64-mingw32.static-windres icon.rc -O coff -o icon.res
    SET( ICON_RESOURCE "${CMAKE_SOURCE_DIR}/misc/icon.res" )
endif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

SET( APPDATA_DIR "${CMAKE_BINARY_DIR}/appdata" )
SET( LUASCRIPTS_DIR "${APPDATA_DIR}/lua" )

add_custom_target( create-runtime-files-dir
    COMMAND ${MKDIR} -p "${CMAKE_BINARY_DIR}/appdata/uischemes" )
add_custom_target( copy-runtime-files ALL
    COMMAND ${RSYNC} ${RSYNC_ARGS} "${CMAKE_SOURCE_DIR}/src/uischemes/" "${CMAKE_BINARY_DIR}/appdata/uischemes/"
    DEPENDS create-runtime-files-dir )
add_custom_target( copy-runtime-files-ex-data ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/exampleData/ ${CMAKE_BINARY_DIR}/exampleData/ )
add_custom_target( copy-lua-plumbing ALL
    COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/${PLUMBING_DIR}/plumbing.lua ${LUASCRIPTS_DIR}/plumbing.lua )
add_custom_target( copy-lua-debug ALL
    COMMAND cmake -E copy_if_different ${CMAKE_SOURCE_DIR}/${PLUMBING_DIR}/mobdebug.lua ${LUASCRIPTS_DIR}/mobdebug.lua )
add_custom_target( copy-lua-scripts ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/src/lua ${LUASCRIPTS_DIR} )
add_custom_target( copy-root-data ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/rootData/ ${CMAKE_BINARY_DIR} )
add_custom_target( create-gtk-schemas-dir
    COMMAND ${MKDIR} -p "${CMAKE_BINARY_DIR}/appdata/usr/share/glib-2.0/schemas" )
add_custom_target( copy-gtk-schemas ALL
    COMMAND ${RSYNC} ${RSYNC_ARGS} "${CMAKE_PREFIX_PATH}/share/glib-2.0/schemas/" "${CMAKE_BINARY_DIR}/appdata/usr/share/glib-2.0/schemas/"
    DEPENDS create-gtk-schemas-dir )
add_custom_target( copy-gtk-icons ALL
    COMMAND ${RSYNC} ${RSYNC_ARGS} "${CMAKE_PREFIX_PATH}/share/icons" "${CMAKE_BINARY_DIR}/appdata/usr/share/"
    DEPENDS create-gtk-schemas-dir )
add_custom_target( copy-gtk-themes ALL
    COMMAND ${RSYNC} ${RSYNC_ARGS} "${CMAKE_SOURCE_DIR}/src/themes" "${CMAKE_BINARY_DIR}/appdata/"
    DEPENDS create-gtk-schemas-dir )
add_custom_target( copy-app-data ALL
    COMMAND ${RSYNC} ${RSYNC_ARGS} "${CMAKE_SOURCE_DIR}/appData/" "${CMAKE_BINARY_DIR}/appdata/"
    DEPENDS create-gtk-schemas-dir )
if (NOT SAFE_FILE_DOWNLOADER_PREBUILT)
    add_custom_target( build-safe-dl-build ALL
        COMMAND CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/rustlib ${CARGO_COMMAND}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/rust/safe_file_downloader )
    add_custom_target( build-safe-dl ALL
        COMMAND ${FLATGLOBCOPY} ${RUST_LIB_DIR} "safefiledownloader\\\\\\.\\(so\\|dll\\|a\\)" ${RUST_LINK_DIR}
        DEPENDS build-safe-dl-build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/rust/safe_file_downloader )
else()
    add_custom_target( build-safe-dl ALL
        COMMAND ${RSYNC} "${SAFE_FILE_DOWNLOADER_PREBUILT}" "${RUST_LINK_DIR}/" )
endif()

add_custom_target( version-info
    COMMAND ${CMAKE_SOURCE_DIR}/buildutil/genversion
    "${CMAKE_SOURCE_DIR}" "${CMAKE_MXE_HASH}"
    "${SAFELISTS_VERSION}" "${CMAKE_BINARY_DIR}/version.json" )

add_custom_target( package-zip
    COMMAND ${CMAKE_SOURCE_DIR}/buildutil/packagestuff
        ${CMAKE_SOURCE_DIR}/buildutil/files-to-package.txt
        ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/package
    DEPENDS version-info )

file(GLOB SCRYPT_SOURCES ${LIBSCRYPT_DIR}/*.c )
set_source_files_properties( ${SCRYPT_SOURCES} PROPERTIES LANGUAGE CXX )
set_source_files_properties( ${SCRYPT_SOURCES} PROPERTIES COMPILE_FLAGS "-xc -std=c99" )

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET( LINUX_EXTRA_LIBS "dl" )
endif()

add_library( domain ${DOMAIN_SOURCES} )
target_link_libraries( domain
    sqlite3 pthread plumbing
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    safefiledownloader
    sodium
    ${LINUX_EXTRA_LIBS}
)

add_dependencies( domain build-safe-dl )

SET( TEST_SOURCES
    tests/model-tests.cpp
    tests/io-tests.cpp
    tests/interval-tests.cpp
    tests/downloader-tests.cpp
    tests/util-tests.cpp
    tests/graceful-shutdown-tests.cpp
)

SET( DUMB_HASH_SOURCES
    src/dumbhash.cpp
)

add_executable( all-tests ${TEST_SOURCES} )
target_link_libraries( all-tests domain )

add_executable( dumbhash256 ${DUMB_HASH_SOURCES} )
target_link_libraries( dumbhash256 domain )

add_library( gtkspec ${GTK_SOURCES} )
target_link_libraries( gtkspec domain ${GTKMM_LIBRARIES} )

add_executable( safelists src/main.cpp )
target_link_libraries( safelists gtkspec ${ICON_RESOURCE} )
set_target_properties( safelists PROPERTIES LINK_FLAGS "-Wl,-rpath -Wl,$ORIGIN" )

