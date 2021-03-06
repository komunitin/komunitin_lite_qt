cmake_minimum_required(VERSION 3.5)

project(komunitin_lite VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Network Widgets StateMachine LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network Widgets StateMachine LinguistTools)

set(TS_FILES ts/komunitin_lite_ca_ES.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/kstatemachine.h
        src/kstatemachine.cpp
        src/logindialog.ui
        src/transferdialog.ui
        src/account.h
        src/account.cpp
        src/transfer.h
        src/transfer.cpp
        src/oauth2.h
        src/oauth2.cpp
        src/netservices.h
        src/netservices.cpp
        src/logindialog.h
        src/logindialog.cpp
        src/transferdialog.h
        src/transferdialog.cpp
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(komunitin_lite
        MANUAL_FINALIZATION
        svg/icons.qrc
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET komunitin_lite APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(komunitin_lite SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(komunitin_lite
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(komunitin_lite PRIVATE Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::StateMachine)

set_target_properties(komunitin_lite PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER lite.komunitin.org
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(komunitin_lite)
endif()
