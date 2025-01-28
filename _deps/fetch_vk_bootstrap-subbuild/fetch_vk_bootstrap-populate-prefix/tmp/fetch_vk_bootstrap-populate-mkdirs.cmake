# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-src")
  file(MAKE_DIRECTORY "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-src")
endif()
file(MAKE_DIRECTORY
  "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-build"
  "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix"
  "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix/tmp"
  "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix/src/fetch_vk_bootstrap-populate-stamp"
  "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix/src"
  "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix/src/fetch_vk_bootstrap-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix/src/fetch_vk_bootstrap-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/dp/Desktop/CS_450_paviad/_deps/fetch_vk_bootstrap-subbuild/fetch_vk_bootstrap-populate-prefix/src/fetch_vk_bootstrap-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
