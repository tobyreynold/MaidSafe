#==================================================================================================#
#                                                                                                  #
#  Copyright 2012 MaidSafe.net limited                                                             #
#                                                                                                  #
#  This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,        #
#  version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which    #
#  licence you accepted on initial access to the Software (the "Licences").                        #
#                                                                                                  #
#  By contributing code to the MaidSafe Software, or to this project generally, you agree to be    #
#  bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root        #
#  directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available   #
#  at: http://www.maidsafe.net/licenses                                                            #
#                                                                                                  #
#  Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed    #
#  under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF   #
#  ANY KIND, either express or implied.                                                            #
#                                                                                                  #
#  See the Licences for the specific language governing permissions and limitations relating to    #
#  use of the MaidSafe Software.                                                                   #
#                                                                                                  #
#==================================================================================================#


if(APPLE)
  cmake_minimum_required(VERSION 2.8.12.2 FATAL_ERROR)
else()
  cmake_minimum_required(VERSION 2.8.12.1 FATAL_ERROR)
endif()

if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  set(Msg "\n\nThis project doesn't allow \"in-source builds\".\nYou must run CMake from a folder outside")
  set(Msg "${Msg} the root of this project.\nFor more details about configuring the project see\n")
  if(WIN32)
    set(Msg "${Msg}https://github.com/maidsafe/MaidSafe/wiki/Build-Instructions-for-Windows")
  elseif(APPLE)
    set(Msg "${Msg}https://github.com/maidsafe/MaidSafe/wiki/Build-Instructions-for-OS-X")
  else()
    set(Msg "${Msg}https://github.com/maidsafe/MaidSafe/wiki/Build-Instructions-for-Linux")
  endif()
  message(WARNING "${Msg}\n\n")
  message(STATUS "Cancelling CMake and cleaning up source tree...")
  execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/CMakeFiles")
  # This next call should cause CMake to crash.  We should remove this dirty hack if CMake becomes
  # able to be cancelled in a clean way (i.e. doesn't leave behind files/folders).
  math(EXPR Crash 0/0)
  message(FATAL_ERROR "CMake should have crashed - this is a failsafe in case the call used to trigger the crash gets fixed.")
endif()

project(maidsafe)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type given.  Setting CMAKE_BUILD_TYPE to Debug.")
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(CTest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # works for Unix MakeFiles (also ninja)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_ERROR_DEPRECATED ON)
set(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION ON)
set(EXPORT_COMPILE_COMMANDS ON)
set(HR "================================================================================")


set(SubModules
      common
      passport
      rudp
      routing
      encrypt
      nfs
      vault
      vault_manager
      drive
      api)
# Set all submodules' source dir before calling add_subdirectory to avoid
# having to run CMake twice in a clean build tree.
foreach(SubModule ${SubModules})
  set(${SubModule}_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/${SubModule})
endforeach()

# Check the submodules are initialised
foreach(SubModule ${SubModules})
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/${SubModule}/.git")
    set(ErrorMsg "\nThe git submodule for ${SubModule} is not intialised.  ")
    set(ErrorMsg "${ErrorMsg}Please cd to ${CMAKE_SOURCE_DIR} and run:\n    ")
    set(ErrorMsg "${ErrorMsg}git submodule update --init --recursive\n")
    message(FATAL_ERROR ${ErrorMsg})
  endif()
endforeach()


set(CMAKE_MODULE_PATH ${maidsafe_SOURCE_DIR}/cmake_modules)

include(version)

# Set Maidsafe Testing to OFF if package build defined.
if(PACKAGE_BUILD)
  set(MaidsafeTesting OFF)
else()
  set(MaidsafeTesting ON)
endif()


# Force all libraries and exes to be built to the top-level build dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})


# Include helper functions, check compiler version, and get target platform details
include(utils)
ms_check_compiler()
ms_get_target_platform()
ms_get_target_architecture()


# Adds a target 'CleanCoverage' on Unix.  When this target is built, it removes
# all .gcda files from the build directory and its subdirectories
if(UNIX)
  find_file(CleanCoverageFile clean_coverage.cmake cmake_modules)
  add_custom_target(CleanCoverage COMMAND ${CMAKE_COMMAND} -DSearchDir=${CMAKE_BINARY_DIR} -P ${CleanCoverageFile})
  option(COVERAGE "If ON, targets will be built with coverage flags included." OFF)
endif()


# Add build types for the Address, Memory, Thread and Undefined Behaviour Sanitizers
include(find_sanitize_blacklist)
include(find_asan)
include(find_msan)
include(find_tsan)
include(find_ubsan)
include(find_stack_protector_strong)


include(maidsafe_find_just_thread)
include(maidsafe_find_python)
include(maidsafe_find_git)
include(maidsafe_find_openmp)

if(APPLE)
  option(HAVE_LIBC++ "If ON, '-stdlib=libc++' is added to the compile and link flags" ON)
else()
  option(HAVE_LIBC++ "If ON, '-stdlib=libc++' is added to the compile and link flags" OFF)
  option(HAVE_LIBC++ABI "If ON, '-lc++abi' is added to the link flags" OFF)
endif()
option(USE_BOOST_CACHE "If ON, try to download, extract and build boost to a single cache directory for all MaidSafe clones" OFF)
option(BOOST_DISABLE_ASSERTS "If ON, all BOOST_ASSERTs are disabled" ON)
option(USE_JUST_THREADS "If ON, try to find and use the just::thread library (Windows or gcc 4.7 only)" OFF)
include(standard_flags)


add_subdirectory(src/third_party_libs)


# Add each sub-module
foreach(SubModule ${SubModules})
  add_subdirectory(src/${SubModule})
endforeach()

if(EXISTS ${CMAKE_SOURCE_DIR}/src/sandbox)
  message("${HR}\nConfiguring Sandbox projects\n----------------------------")
  file(GLOB AllSandboxDirs "${maidsafe_SOURCE_DIR}/src/sandbox/*")
  list(REMOVE_ITEM AllSandboxDirs "${maidsafe_SOURCE_DIR}/src/sandbox/.git"
                                  "${maidsafe_SOURCE_DIR}/src/sandbox/.gitignore"
                                  "${maidsafe_SOURCE_DIR}/src/sandbox/README")
  foreach(SandboxDir ${AllSandboxDirs})
    get_filename_component(DirName ${SandboxDir} NAME)
    message(STATUS "${DirName}")
    add_subdirectory(${SandboxDir})
  endforeach()
endif()

message("${HR}")

if(UNIX)
  if(COVERAGE)
    message(STATUS "Coverage ON")
  else()
    message(STATUS "Coverage OFF.  To enable, re-run CMake with -DCOVERAGE=ON")
  endif()
endif()

message(STATUS "To include all GTests,                   cmake . -DMAIDSAFE_TEST_TYPE=ALL")
message(STATUS "To include only behavioural GTests,      cmake . -DMAIDSAFE_TEST_TYPE=BEH")
message(STATUS "To include only functional GTests,       cmake . -DMAIDSAFE_TEST_TYPE=FUNC")
message("${HR}")


# Tidy CTestCustom.cmake
file(STRINGS ${CMAKE_BINARY_DIR}/CTestCustom.cmake CTestCustomContents)
list(REMOVE_DUPLICATES CTestCustomContents)
list(SORT CTestCustomContents)
string(REPLACE ";" "\n" CTestCustomContents "${CTestCustomContents}")
file(WRITE ${CMAKE_BINARY_DIR}/CTestCustom.cmake "${CTestCustomContents}\n")

ms_cleanup_temp_dir()
ms_rename_outdated_built_exes()


unset(MAKECOMMAND CACHE)
ms_setup_ci_scripts()
unset(QA_BUILD CACHE)


# Convenience feature - adds all CMake related files to folder in VS IDE
if(MSVC OR XCODE)
  set(RootFiles ${CMAKE_SOURCE_DIR}/CI.cmake.in
                ${CMAKE_SOURCE_DIR}/CMakeLists.txt
                ${CMAKE_SOURCE_DIR}/CTestConfig.cmake
                ${CMAKE_SOURCE_DIR}/Project.xml)
  source_group("root" FILES ${RootFiles})

  source_group("root\\src\\third_party_libs" FILES ${CMAKE_SOURCE_DIR}/src/third_party_libs/CMakeLists.txt)

  file(GLOB ModuleFiles ${CMAKE_SOURCE_DIR}/cmake_modules/*)
  source_group("root\\cmake_modules" FILES ${ModuleFiles})

  add_custom_target(CMakeFiles SOURCES ${RootFiles} src/third_party_libs/CMakeLists.txt ${ModuleFiles})
endif()
