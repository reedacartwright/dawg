# This CMake module finds packages and, if not found, builds them
include(ExternalProject)

# Prelim settings
SET(EXT_PREFIX ext_deps)

IF(NOT "${CMAKE_VERSION}" VERSION_LESS 3.2)
  SET(use_byproducts true)
ENDIF()

SET(EXT_CFLAGS "${CMAKE_C_FLAGS}")
SET(EXT_LDFLAGS "${CMAKE_STATIC_LINKER_FLAGS}")
IF(CMAKE_BUILD_TYPE)
  STRING(TOUPPER "${CMAKE_BUILD_TYPE}" cmake_build_type_toupper)
  SET(EXT_CFLAGS "${EXT_CFLAGS} ${CMAKE_C_FLAGS_${cmake_build_type_toupper}}")
  SET(EXT_LDFLAGS "${EXT_LDFLAGS} ${CMAKE_STATIC_LINKER_FLAGS_${cmake_build_type_toupper}}")

  ## Turn off debugging in libraries
  IF(cmake_build_type_toupper STREQUAL "RELEASE" OR
     cmake_build_type_toupper STREQUAL "RELWITHDEBINFO")
    ADD_DEFINITIONS("-DBOOST_DISABLE_ASSERTS")
    SET(boost_variant variant=release)
  ELSE()
    SET(boost_variant variant=debug)
  ENDIF()
ENDIF()

IF(NOT BUILD_EXTERNAL_PROJECTS)
  SET(REQ REQUIRED)
  SET(QUI )
ELSE()
  SET(REQ )
  SET(QUI QUIET)
  STRING(TOUPPER "${BUILD_EXTERNAL_PROJECTS}" build_external_projects_toupper)
  IF(build_external_projects_toupper STREQUAL "FORCE")
    SET(BUILD_EXTERNAL_PROJECTS_FORCED ON)
  ENDIF()
ENDIF()
SET(missing_ext_deps FALSE)

IF(USE_STATIC_LIBS)
  SET(Boost_USE_STATIC_LIBS ON)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF(USE_STATIC_LIBS)

ADD_CUSTOM_TARGET(ext_projects)

################################################################################
# THREADS
#

SET(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads)

################################################################################

################################################################################
# BOOST
#

IF(NOT BUILD_EXTERNAL_PROJECTS_FORCED)
  #IF(DEVEL_MODE)
  #  SET(boost_devel timer)
  #ENDIF()
  FIND_PACKAGE(Boost 1.47.0 ${REQ} COMPONENTS
    program_options
    unit_test_framework
    #${boost_devel}
  )

  IF(Boost_FOUND)
  	message(STATUS "Boost library: ${Boost_LIBRARY_DIRS}")
  	message(STATUS "Boost headers: ${Boost_INCLUDE_DIRS}")
  ENDIF(Boost_FOUND)
ENDIF()

IF(BUILD_EXTERNAL_PROJECTS AND NOT Boost_FOUND)
  SET(boost_ext_libdir "${CMAKE_BINARY_DIR}/${EXT_PREFIX}/boost/lib")
  SET(Boost_FOUND TRUE)
  SET(Boost_VERSION 1.60.0)
  SET(Boost_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/${EXT_PREFIX}/boost/include/")
  FILE(MAKE_DIRECTORY "${Boost_INCLUDE_DIRS}")
  SET(Boost_LIBRARY_DIRS "")
  SET(BOOST_EXT_TARGET ext_boost)
  SET(Boost_USE_STATIC_LIBS TRUE)

  SET(Boost_LIBRARIES)
  FOREACH(ext_boost_name PROGRAM_OPTIONS UNIT_TEST_FRAMEWORK)
    STRING(TOLOWER "${ext_boost_name}" ext_boost_lowname)
    SET(Boost_${ext_boost_name}_FOUND On)
    SET(Boost_${ext_boost_name}_LIBRARY "${boost_ext_libdir}/libboost_${ext_boost_lowname}.a")
    SET(Boost_LIBRARIES ${Boost_LIBRARIES} ${Boost_${ext_boost_name}_LIBRARY})
  ENDFOREACH()

  IF(use_byproducts)
    SET(byproducts BUILD_BYPRODUCTS ${Boost_LIBRARIES})
  ENDIF()

  IF(CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$")
    SET(EXT_BOOST_CXX_TOOLSET "clang")
  ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(EXT_BOOST_CXX_TOOLSET "gcc")
  ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    SET(EXT_BOOST_CXX_TOOLSET "intel")
  ELSE()
    SET(EXT_BOOST_CXX_TOOLSET "cc")
  ENDIF()

  IF(NOT DEFINED EXT_BOOST_TOOLSET)
    SET(EXT_BOOST_TOOLSET "${EXT_BOOST_CXX_TOOLSET}-${CMAKE_CXX_COMPILER_VERSION}" CACHE STRING "Toolset to use when building ext_boost.")  
  ENDIF()

  IF(EXT_BOOST_TOOLSET)
    SET(boost_toolset "toolset=${EXT_BOOST_TOOLSET}")
  ENDIF()

  SET(EXT_BOOST_BOOTSTRAP_TOOLSET "cc" CACHE STRING "Toolset to use when bootstrapping ext_boost.")

  CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/Modules/cmake_ext_boost_bootstrap.cmake.in"
    "${CMAKE_BINARY_DIR}/${EXT_PREFIX}/cmake_ext_boost_bootstrap.cmake"
    IMMEDIATE @ONLY
  )

  SET(boost_bootstrap
    "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/${EXT_PREFIX}/cmake_ext_boost_bootstrap.cmake"
  )

  MARK_AS_ADVANCED(EXT_BOOST_TOOLSET EXT_BOOST_BOOTSTRAP_TOOLSET)

  IF(cxx_std_flag)
    SET(boost_cxxflags "cxxflags=${cxx_std_flag}")
  ENDIF()

  SET(boost_build
    ./b2 install
    --prefix=<INSTALL_DIR>
    --with-program_options
    --with-test
    --disable-icu
    --ignore-site-config
    threading=multi
    link=static
    runtime-link=shared
    optimization=speed
    ${boost_toolset}
    ${boost_cxxflags}
    ${boost_variant}
  )

  string(REPLACE "." "_" boost_file_version "${Boost_VERSION}")

  ExternalProject_add(ext_boost
    URL http://downloads.sourceforge.net/project/boost/boost/${Boost_VERSION}/boost_${boost_file_version}.tar.bz2
    URL_MD5 65a840e1a0b13a558ff19eeb2c4f0cbe
    PREFIX "${EXT_PREFIX}/boost"
    BUILD_IN_SOURCE TRUE
    CONFIGURE_COMMAND ${boost_bootstrap}
    BUILD_COMMAND ${boost_build}
    INSTALL_COMMAND ""
    ${byproducts}
  )
  ADD_DEPENDENCIES(ext_projects ext_boost)

  MESSAGE(STATUS "Building Boost ${Boost_VERSION} as external dependency")  
ENDIF()

IF(Boost_FOUND)
  FOREACH(ext_boost_name PROGRAM_OPTIONS UNIT_TEST_FRAMEWORK)
    if(Boost_${ext_boost_name}_FOUND AND NOT TARGET Boost::${ext_boost_name})
      add_library(Boost::${ext_boost_name} UNKNOWN IMPORTED)
      set_target_properties(Boost::${ext_boost_name} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${Boost_${ext_boost_name}_LIBRARY}"
      )
      if(BOOST_EXT_TARGET)
        ADD_DEPENDENCIES(Boost::${ext_boost_name} ext_boost)
      endif()
    endif()  
  ENDFOREACH()
  ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB -DBOOST_PROGRAM_OPTIONS_NO_LIB)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
  IF(NOT Boost_USE_STATIC_LIBS)
    ADD_DEFINITIONS(-DBOOST_DYN_LINK -DBOOST_PROGRAM_OPTIONS_DYN_LINK -DBOOST_TEST_DYN_LINK)
  ENDIF(NOT Boost_USE_STATIC_LIBS)
ELSE()
  SET(missing_ext_deps TRUE)
ENDIF(Boost_FOUND)