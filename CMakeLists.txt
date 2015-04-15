cmake_minimum_required(VERSION 2.8)

project( OttDate )

set( ${PROJECT_NAME}_MAJOR_VERSION 0 )
set( ${PROJECT_NAME}_MINOR_VERSION 1 )
set( ${PROJECT_NAME}_PATCH_VERSION 0 )
set( ${PROJECT_NAME}_VERSION ${PROJECT_NAME}_MAJOR_VERSION.${PROJECT_NAME}_MINOR_VERSION.${PROJECT_NAME}_PATCH_VERSION )

find_package( OpenSSL REQUIRED )

include_directories( "${PROJECT_SOURCE_DIR}/library" )
include_directories( "${PROJECT_SOURCE_DIR}" )
include_directories( "${PROJECT_BINARY_DIR}" )

message( "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}" )
message( "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}" )


add_subdirectory(example)
add_subdirectory(library)


export(TARGETS ottdate ottdate_static FILE "${PROJECT_BINARY_DIR}/OttDateTargets.cmake" )
export(PACKAGE OttDate)

#... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(OttDateConfig.cmake.in "${PROJECT_BINARY_DIR}/OttDateConfig.cmake" @ONLY)

# ... for the install tree
set(CONF_INCLUDE_DIRS "\${FOOBAR_CMAKE_DIR}/include")
configure_file(OttDateConfig.cmake.in "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/OttDateConfig.cmake" @ONLY)

# ... for both
configure_file(OttDateConfigVersion.cmake.in "${PROJECT_BINARY_DIR}/OttDateConfigVersion.cmake" @ONLY)

# Install the OttDateConfig.cmake and OttDateConfigVersion.cmake
install(
  FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/OttDateConfig.cmake"
  "${PROJECT_BINARY_DIR}/OttDateConfigVersion.cmake"
  DESTINATION lib/CMake/OttDate COMPONENT dev)

# Install the export set for use with the install-tree
install(EXPORT OttDateTargets DESTINATION lib/CMake/OttDate COMPONENT dev)
