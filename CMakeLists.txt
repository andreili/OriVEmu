cmake_minimum_required(VERSION 3.16)

project(OriVEmu VERSION 0.1 LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Verilator REQUIRED)

add_library(Vemulate STATIC sim_wrappers/sim_top.cpp
    .gitignore
    sim_common/common/tb.h
    sim_common/common/tb.cpp
    sim_wrappers/sim_top.cpp
    sim_wrappers/sim_top.h)

add_definitions(-DTOP_NAME=emulate -DTO_SIM)
target_include_directories(Vemulate PRIVATE sim_common/common)
target_include_directories(Vemulate PRIVATE /usr/include/qt${QT_VERSION_MAJOR})
target_include_directories(Vemulate PRIVATE /usr/include/qt${QT_VERSION_MAJOR}/QtCore)
set_property(TARGET Vemulate PROPERTY POSITION_INDEPENDENT_CODE ON)

verilate(Vemulate
    PREFIX Vemulate
    INCLUDE_DIRS "rtl"
    INCLUDE_DIRS "rtl/TV80-verilog"
    SOURCES rtl/emulate.sv
    THREADS 1
    TOP_MODULE emulate)
#  -x-assign fast -Wall
#VERILATOR_ARGS --timescale-override 1ps/1ps
# --no-timing

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
        gui/main.cpp
        gui/mainwindow.cpp
        gui/mainwindow.h
        gui/mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(OriVEmu
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET OriVEmu APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(OriVEmu SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(OriVEmu
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(OriVEmu PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(OriVEmu PRIVATE Vemulate)
target_include_directories(OriVEmu PRIVATE sim_wrappers)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.OriVEmu)
endif()
set_target_properties(OriVEmu PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS OriVEmu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(OriVEmu)
endif()
