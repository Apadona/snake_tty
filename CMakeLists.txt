cmake_minimum_required( VERSION 3.16 )

project( snake )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CXX_STANDARD 17 )

option( architecture_64 "select build architecture (32 or 64) bit." ON )

set( project_source_directory "${CMAKE_SOURCE_DIR}/src" )
set( project_binary_directory "${CMAKE_SOURCE_DIR}/bin" )

set( release_compile_flags "-O2 -Wall -Wextra -s" )
set( debug_compile_flags " -g3 -Wall -Wextra" )

if( WIN32 )
    set( target_os "windows" )

elseif( UNIX )
    if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
        set( target_os "linux" )

    else()
        message( FATAL_ERROR "Unix platform other than Linux is not supported!\n" )
    endif()

else()
    message( FATAL_ERROR "Only windows and Linux are supported.\n" )

endif()

if( architecture_64 )
    set( target_architecture "64" )

else()
    set( target_architecture "32" )

endif()

set( RUNTIME_OUTPUT_DIRECTORY "${project_binary_directory}/${target_os}/${target_architecture}" )

file ( GLOB_RECURSE project_source_files
                    "${project_source_directory}/*.cpp"
     )

add_executable( snake ${project_source_files} )

set_target_properties( snake PROPERTIES
                             RUNTIME_OUTPUT_DIRECTORY "${RUNTIME_OUTPUT_DIRECTORY}" )

if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    set_target_properties( snake PROPERTIES
                            COMPILE_FLAGS "-m${target_architecture} ${debug_compile_flags}"
                         )
endif()

if( CMAKE_BUILD_TYPE STREQUAL "Release" )
    set_target_properties( snake PROPERTIES
                            COMPILE_FLAGS "-m${target_architecture} ${release_compile_flags}"
                         )
endif()                        