cmake_minimum_required(VERSION 3.11)
project(litebrowser VERSION "0.1.0" LANGUAGES CXX)
include(FetchContent)
include(CMakeFuncs.txt)
if (UNIX)
    find_package(PkgConfig REQUIRED)
    set(SHLWAPI)
else()
    find_library(SHLWAPI Shlwapi.lib) 
endif()

option(NO_TOOLBAR "No Toolbar" OFF)

# build: release build
#if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
#    set(CMAKE_BUILD_TYPE "Release")
#    message(STATUS "Build type not specified: defaulting to release")
#endif()

# module: add StereoKit
set(SK_BUILD_TESTS       OFF CACHE INTERNAL "")
set(SK_BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
FetchContent_Declare(StereoKitC GIT_REPOSITORY https://github.com/StereoKit/StereoKit.git GIT_TAG v0.3.6)
FetchContent_MakeAvailable(StereoKitC)
set(StereoKitTools ${stereokitc_SOURCE_DIR}/Tools)

# module: add cairo
if (UNIX)
    pkg_check_modules(LB_LIBS REQUIRED cairo)
else()
    FetchContent_Declare(cairo GIT_REPOSITORY https://github.com/tordex/cairo)
    FetchContent_GetProperties(cairo)
    if(NOT cairo_POPULATED)
	    FetchContent_Populate(cairo)
	    add_library(cairo SHARED IMPORTED)
	    set_target_properties(cairo PROPERTIES IMPORTED_LOCATION ${cairo_SOURCE_DIR}/64/cairo.lib)
	    set_target_properties(cairo PROPERTIES IMPORTED_IMPLIB   ${cairo_SOURCE_DIR}/64/cairo.lib)
	    target_include_directories(cairo INTERFACE ${cairo_SOURCE_DIR}/src)
    endif()
    set(LB_LIBS_INCLUDE_DIRS ${cairo_SOURCE_DIR}/src)
    set(LB_LIBS_LIBRARIES cairo)
endif()

# module: add txdib simpledib
FetchContent_Declare(txdib GIT_REPOSITORY https://github.com/tordex/txdib)
FetchContent_Declare(simpledib GIT_REPOSITORY https://github.com/tordex/simpledib)
FetchContent_MakeAvailable(txdib simpledib)
set(LB_LIBS_INCLUDE_DIRS ${LB_LIBS_INCLUDE_DIRS} ${txdib_SOURCE_DIR} ${simpledib_SOURCE_DIR})
set(LB_LIBS_LIBRARIES ${LB_LIBS_LIBRARIES} txdib simpledib)

# module: add litehtml
set(BUILD_TESTING 0)
add_subdirectory(litehtml)

# set source and headers
set(CONTAINERS litehtml/containers)
create_resource_file(ResourceMap src/html/* src/html.cpp)
set(SOURCE
    #${CONTAINERS}/stereokit/container_stereokit.cpp
    ${CONTAINERS}/cairo/cairo_container.cpp
    ${CONTAINERS}/cairo/cairo_font.cpp
    src/litehtml/el_omnibox.cpp
    src/litehtml/sl_edit.cpp
    src/browser_wnd.cpp
    src/defaults.cpp
    src/html.cpp
    src/htmlview_wnd.cpp
    src/main.cpp
    src/message.cpp
    src/toolbar_wnd.cpp
    src/web_history.cpp
    src/web_page.cpp
)
set(HEADERS
    #${CONTAINERS}/stereokit/container_stereokit.h
    ${CONTAINERS}/cairo/cairo_container.cpp
    ${CONTAINERS}/cairo/cairo_font.cpp
    src/litehtml/el_omnibox.h
    src/litehtml/sl_edit.h
    src/browser_wnd.h
    src/defaults.h
    src/globals.h
    src/htmlview_wnd.h
    src/message.h
    src/toolbar_wnd.h
    src/web_history.h
    src/web_page.h
)
if (UNIX)
    set(SOURCE ${SOURCE}
        src/linux/http_loader.cpp
    )
    set(HEADERS ${HEADERS}
        src/linux/http_loader.h
    )
else()
    set(SOURCE ${SOURCE}
        src/win32/http_loader.cpp
        src/win32/TxThread.cpp
    )
    set(HEADERS ${HEADERS}
        src/win32/http_loader.h
        src/win32/TxThread.h
    )
    set(LB_LIBS_LIBRARIES ${LB_LIBS_LIBRARIES} gdiplus shlwapi Msimg32)
endif()

# executable
add_executable(litebrowser ${SOURCE} ${HEADERS})
target_compile_definitions(litebrowser PUBLIC UNICODE PUBLIC _UNICODE)
if (NO_TOOLBAR)
    target_compile_definitions(litebrowser PUBLIC NO_TOOLBAR)
endif()
include_directories(litebrowser
    ${StereoKitC_SOURCE_DIR}
    ${litehtml_SOURCE_DIR}/include
    ${LB_LIBS_INCLUDE_DIRS})
target_link_options(litebrowser
    PRIVATE ${LB_LIBS_LDFLAGS})
target_link_libraries(litebrowser
    StereoKitC
    litehtml
    ${LB_LIBS_LIBRARIES})
set_target_properties(litebrowser PROPERTIES
    CXX_STANDARD 11
    C_STANDARD 9
)

# msvc: add incremental builds
if(MSVC)
    #target_compile_options(litebrowser PUBLIC "/ZI")
    target_link_options(litebrowser PUBLIC "/INCREMENTAL")
endif()

# build assets
add_custom_command(TARGET litebrowser PRE_BUILD 
    COMMAND "${StereoKitTools}/skshaderc.exe"
    -o3 -t xgew -i "${StereoKitTools}/include"
    -o "${CMAKE_BINARY_DIR}/assets"
    "${litebrowser_SOURCE_DIR}/src/assets/*.hlsl")

# copy dll
add_custom_command(TARGET litebrowser POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${cairo_SOURCE_DIR}/64/cairo.dll"
    $<TARGET_FILE_DIR:litebrowser>)
add_custom_command(TARGET litebrowser POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${txdib_SOURCE_DIR}/../freeimage-src/64/freeimage.dll"
    $<TARGET_FILE_DIR:litebrowser>)
