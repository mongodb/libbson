# Copyright 2017 MongoDB Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Demonstrates how to use the CMake 'FindPkgConfig' mechanism to locate
# and build against libbson.

cmake_minimum_required(VERSION 2.8)

if (APPLE)
   cmake_policy(SET CMP0042 OLD)
endif()

project(hello_bson LANGUAGES C)

find_package(PkgConfig REQUIRED)

# NOTE: For this to work, the PKG_CONFIG_PATH variable must be set to point to
# the directory that was used as the argument to CMAKE_INSTALL_PREFIX when
# building libbson.
pkg_search_module(BSON REQUIRED libbson-static-1.0)

link_directories(${BSON_LIBRARY_DIRS})
add_executable(hello_bson ../hello_bson.c)
target_include_directories(hello_bson PRIVATE ${BSON_INCLUDE_DIRS})
target_link_libraries(hello_bson PRIVATE ${BSON_LIBRARIES} pthread)
