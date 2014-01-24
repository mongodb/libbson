cmake_minimum_required(VERSION 2.8)

project (libbson)

include(TestBigEndian)
include(InstallRequiredSystemLibraries)

set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/../..")

set (BSON_MAJOR_VERSION 0)
set (BSON_MINOR_VERSION 4)
set (BSON_MICRO_VERSION 1)
set (BSON_API_VERSION 1.0)
set (BSON_VERSION 0.4.1)

set (CPACK_RESOURCE_FILE_LICENSE "${SOURCE_DIR}/COPYING")
set (CPACK_PACKAGE_VERSION_MAJOR ${BSON_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${BSON_MINOR_VERSION})

include (CPack)

add_definitions(-DBSON_COMPILATION)

TEST_BIG_ENDIAN(BSON_BIG_ENDIAN)

if (MSVC)
   set (BSON_OS 2)
else ()
   find_package (Threads)

   set (BSON_OS 1)
endif ()

if (BSON_BIG_ENDIAN)
   set (BSON_BYTE_ORDER 4321)
else ()
   set (BSON_BYTE_ORDER 1234)
endif ()

configure_file (
   "${SOURCE_DIR}/bson/bson-config.h.in"
   "${PROJECT_BINARY_DIR}/bson/bson-config.h"
)

configure_file (
   "${SOURCE_DIR}/bson/bson-version.h.in"
   "${PROJECT_BINARY_DIR}/bson/bson-version.h"
)

configure_file (
   "${PROJECT_SOURCE_DIR}/bson/bson-stdint.h"
   "${PROJECT_BINARY_DIR}/bson/bson-stdint.h"
)

include_directories("${SOURCE_DIR}/bson")
include_directories("${PROJECT_BINARY_DIR}/bson")

set (SOURCES
   ${SOURCE_DIR}/bson/bcon.c
   ${SOURCE_DIR}/bson/bson.c
   ${SOURCE_DIR}/bson/bson-clock.c
   ${SOURCE_DIR}/bson/bson-compat.c
   ${SOURCE_DIR}/bson/bson-context.c
   ${SOURCE_DIR}/bson/bson-error.c
   ${SOURCE_DIR}/bson/bson-iter.c
   ${SOURCE_DIR}/bson/bson-keys.c
   ${SOURCE_DIR}/bson/bson-md5.c
   ${SOURCE_DIR}/bson/bson-memory.c
   ${SOURCE_DIR}/bson/bson-oid.c
   ${SOURCE_DIR}/bson/bson-reader.c
   ${SOURCE_DIR}/bson/bson-string.c
   ${SOURCE_DIR}/bson/bson-utf8.c
   ${SOURCE_DIR}/bson/bson-writer.c
)

set (HEADERS
   ${PROJECT_BINARY_DIR}/bson/bson-config.h
   ${PROJECT_BINARY_DIR}/bson/bson-stdint.h
   ${PROJECT_BINARY_DIR}/bson/bson-version.h
   ${SOURCE_DIR}/bson/bcon.h
   ${SOURCE_DIR}/bson/bson-clock.h
   ${SOURCE_DIR}/bson/bson-compat.h
   ${SOURCE_DIR}/bson/bson-context.h
   ${SOURCE_DIR}/bson/bson-endian.h
   ${SOURCE_DIR}/bson/bson-error.h
   ${SOURCE_DIR}/bson/bson.h
   ${SOURCE_DIR}/bson/bson-iter.h
   ${SOURCE_DIR}/bson/bson-keys.h
   ${SOURCE_DIR}/bson/bson-macros.h
   ${SOURCE_DIR}/bson/bson-md5.h
   ${SOURCE_DIR}/bson/bson-memory.h
   ${SOURCE_DIR}/bson/bson-oid.h
   ${SOURCE_DIR}/bson/bson-reader.h
   ${SOURCE_DIR}/bson/bson-string.h
   ${SOURCE_DIR}/bson/bson-thread.h
   ${SOURCE_DIR}/bson/bson-types.h
   ${SOURCE_DIR}/bson/bson-utf8.h
   ${SOURCE_DIR}/bson/bson-writer.h
)

add_library(bson_shared SHARED ${SOURCES} ${HEADERS})
add_library(bson_static STATIC ${SOURCES} ${HEADERS})

set_target_properties(bson_shared PROPERTIES VERSION ${BSON_VERSION} SOVERSION ${BSON_MAJOR_VERSION})
set_target_properties(bson_static PROPERTIES VERSION ${BSON_VERSION} SOVERSION ${BSON_MAJOR_VERSION})

set_target_properties(bson_shared PROPERTIES OUTPUT_NAME "bson-${BSON_API_VERSION}" PREFIX "lib")
set_target_properties(bson_static PROPERTIES OUTPUT_NAME "bson-${BSON_API_VERSION}")

if (UNIX)
    target_link_libraries (bson_shared ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries (bson_static ${CMAKE_THREAD_LIBS_INIT})
else()
    target_link_libraries (bson_shared ws2_32)
    target_link_libraries (bson_static ws2_32)
endif()

enable_testing()

function(bson_add_test test)
   add_executable(${test} ${ARGN})
   target_link_libraries(${test} bson_static)
   add_test(NAME ${test} COMMAND ${test})
endfunction()

bson_add_test(test-bcon-basic ${SOURCE_DIR}/tests/test-bcon-basic.c)
bson_add_test(test-bcon-extract ${SOURCE_DIR}/tests/test-bcon-extract.c)
bson_add_test(test-bson-clock ${SOURCE_DIR}/tests/test-bson-clock.c)
bson_add_test(test-bson-endian ${SOURCE_DIR}/tests/test-endian.c)
bson_add_test(test-bson-error ${SOURCE_DIR}/tests/test-bson-error.c)
bson_add_test(test-bson-iter ${SOURCE_DIR}/tests/test-bson-iter.c)
bson_add_test(test-bson-json ${SOURCE_DIR}/tests/test-bson-json.c)
bson_add_test(test-bson-oid ${SOURCE_DIR}/tests/test-bson-oid.c)
bson_add_test(test-bson-reader ${SOURCE_DIR}/tests/test-bson-reader.c)
bson_add_test(test-bson-string ${SOURCE_DIR}/tests/test-bson-string.c)
bson_add_test(test-bson ${SOURCE_DIR}/tests/test-bson.c)
bson_add_test(test-bson-utf8 ${SOURCE_DIR}/tests/test-bson-utf8.c)
bson_add_test(test-bson-writer ${SOURCE_DIR}/tests/test-bson-writer.c)

file(COPY ${SOURCE_DIR}/tests/binary DESTINATION ${PROJECT_BINARY_DIR}/tests)

install(
  TARGETS bson_shared bson_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)
install(
  FILES ${HEADERS}
  DESTINATION "include/libbson-${BSON_API_VERSION}"
)
