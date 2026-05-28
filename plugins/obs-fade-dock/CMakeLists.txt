cmake_minimum_required(VERSION 3.16)
project(obs-fade-dock VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt auto-tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Find OBS ──
# If building standalone (out-of-tree), point CMAKE_PREFIX_PATH at your OBS install.
# If building in-tree (inside obs-studio/plugins/), OBS targets are already available.
if(NOT TARGET OBS::libobs)
  find_package(libobs REQUIRED)
endif()
if(NOT TARGET OBS::frontend-api)
  find_package(obs-frontend-api REQUIRED)
endif()

# ── Find Qt ──
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if(NOT Qt6_FOUND)
  find_package(Qt5 5.15 COMPONENTS Core Widgets REQUIRED)
  set(QT_VERSION_MAJOR 5)
else()
  set(QT_VERSION_MAJOR 6)
endif()

# ── Plugin target ──
add_library(obs-fade-dock MODULE
  src/fade-dock.cpp
)

target_link_libraries(obs-fade-dock
  PRIVATE
    OBS::libobs
    OBS::frontend-api
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(obs-fade-dock PRIVATE src)

# ── Install ──
if(UNIX AND NOT APPLE)
  install(TARGETS obs-fade-dock
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/obs-plugins
  )
  install(DIRECTORY data/locale
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/obs-fade-dock
  )
elseif(WIN32)
  install(TARGETS obs-fade-dock
    RUNTIME DESTINATION "obs-plugins/64bit"
  )
  install(DIRECTORY data/locale
    DESTINATION "data/obs-plugins/obs-fade-dock"
  )
elseif(APPLE)
  install(TARGETS obs-fade-dock
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/obs-plugins"
  )
endif()
