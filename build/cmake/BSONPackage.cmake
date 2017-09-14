set (PACKAGE_INCLUDE_INSTALL_DIRS ${BSON_HEADER_INSTALL_DIR})
set (PACKAGE_LIBRARY_INSTALL_DIRS lib)
set (PACKAGE_LIBRARIES bson-1.0)

include (CMakePackageConfigHelpers)

foreach (prefix "1.0" "static-1.0")
   foreach (suffix "config.cmake" "config-version.cmake")
      configure_package_config_file (
         build/cmake/libbson-${prefix}-${suffix}.in
         ${CMAKE_CURRENT_BINARY_DIR}/libbson-${prefix}-${suffix}
         INSTALL_DESTINATION lib/cmake/libbson-${prefix}
         PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
      )

      install (
         FILES
           ${CMAKE_CURRENT_BINARY_DIR}/libbson-${prefix}-${suffix}
         DESTINATION
           lib/cmake/libbson-${prefix}
      )
   endforeach ()
endforeach ()
