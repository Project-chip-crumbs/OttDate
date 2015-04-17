cmake_minimum_required(VERSION 2.8)

project( OttDate )

string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPPER)

set( ${PROJECT_NAME}_MAJOR_VERSION 0 )
set( ${PROJECT_NAME}_MINOR_VERSION 1 )
set( ${PROJECT_NAME}_PATCH_VERSION 0 )
set( ${PROJECT_NAME}_VERSION ${PROJECT_NAME}_MAJOR_VERSION.${PROJECT_NAME}_MINOR_VERSION.${PROJECT_NAME}_PATCH_VERSION )

# -- BEGIN: OttDate specific ---
find_package( OpenSSL REQUIRED )
find_package( Threads REQUIRED )

include_directories( "${PROJECT_SOURCE_DIR}/library" )
include_directories( "${PROJECT_SOURCE_DIR}" )
include_directories( "${PROJECT_BINARY_DIR}" )

add_subdirectory(example)
add_subdirectory(library)

export(TARGETS ottdate ottdate_static FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake" )
export(PACKAGE ${PROJECT_NAME})
# -- END: OttDate specific ---


#... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(${PROJECT_NAME}Config.cmake.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

# ... for the install tree
set(CONF_INCLUDE_DIRS "\${${PROJECT_NAME_UPPER}_CMAKE_DIR}/include")
configure_file(${PROJECT_NAME}Config.cmake.in "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}Config.cmake" @ONLY)

# create ${PROJECT_NAME}ConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${${PROJECT_NAME}_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Install the ${PROJECT_NAME}Config.cmake and ${PROJECT_NAME}ConfigVersion.cmake
install(
  FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}Config.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION lib/CMake/${PROJECT_NAME} COMPONENT dev)

# Install the export set for use with the install-tree
install(EXPORT ${PROJECT_NAME}Targets DESTINATION lib/CMake/${PROJECT_NAME} COMPONENT dev)
