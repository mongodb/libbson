set (PACKAGE_INCLUDE_INSTALL_DIRS ${BSON_HEADER_INSTALL_DIR})
set (PACKAGE_LIBRARY_INSTALL_DIRS lib)
set (PACKAGE_LIBRARIES bson-1.0)

include (CMakePackageConfigHelpers)

configure_package_config_file (
   build/cmake/libbson-1.0-config.cmake.in
   ${CMAKE_CURRENT_BINARY_DIR}/libbson-1.0-config.cmake
   INSTALL_DESTINATION lib/cmake/libbson-1.0
   PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
)

configure_package_config_file (
   build/cmake/libbson-static-1.0-config.cmake.in
   ${CMAKE_CURRENT_BINARY_DIR}/libbson-static-1.0-config.cmake
   INSTALL_DESTINATION lib/cmake/libbson-static-1.0
   PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
)

write_basic_package_version_file (
   ${CMAKE_CURRENT_BINARY_DIR}/libbson-1.0-config-version.cmake
   VERSION ${BSON_VERSION}
   COMPATIBILITY AnyNewerVersion
)

write_basic_package_version_file (
   ${CMAKE_CURRENT_BINARY_DIR}/libbson-static-1.0-config-version.cmake
   VERSION ${BSON_VERSION}
   COMPATIBILITY AnyNewerVersion
)

install (
   FILES
      ${CMAKE_CURRENT_BINARY_DIR}/libbson-1.0-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/libbson-1.0-config-version.cmake
   DESTINATION
      lib/cmake/libbson-1.0
)

install (
   FILES
      ${CMAKE_CURRENT_BINARY_DIR}/libbson-static-1.0-config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/libbson-static-1.0-config-version.cmake
   DESTINATION
      lib/cmake/libbson-static-1.0
)
