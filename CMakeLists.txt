project(pvr.aminopvr)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(curl REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/src)

if(NOT WIN32)
  add_options(ALL_LANGUAGES ALL_BUILDS "-fPIC")
endif()

# Source files
set(AMINOPVR_SOURCES src/client.cpp
                     src/categories.cpp
                     src/AminoPVRData.cpp)

# Header files
set(AMINOPVR_HEADERS src/client.h
                     src/categories.h
                     src/AminoPVRData.h)
source_group("Header Files" FILES ${AMINOPVR_HEADERS})

if(WIN32)
# Misc files
set(RESOURCE_FILES pvr.aminopvr/addon.xml
                   pvr.aminopvr/resources/settings.xml
                   pvr.aminopvr/resources/language/resource.language.en_gb/strings.po)
source_group("Resource Files" FILES ${RESOURCE_FILES})
endif(WIN32)

# Make sure that CMake adds the headers to the MSVC project
list(APPEND AMINOPVR_SOURCES ${AMINOPVR_HEADERS} ${RESOURCE_FILES})

set(DEPLIBS ${p8-platform_LIBRARIES})

find_package(JsonCpp REQUIRED)
list(APPEND DEPLIBS ${JSONCPP_LIBRARIES})
list(APPEND INCLUDES ${JSONCPP_INCLUDE_DIRS})

include_directories(${INCLUDES})

build_addon(pvr.aminopvr AMINOPVR DEPLIBS)

include(CPack)
