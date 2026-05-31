cmake_minimum_required(VERSION 3.13)
project(floral-note VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

# --- Sources ---
set(SOURCES
    src/main.cpp
    src/resources.qrc

    src/app/application.cpp
    src/app/theme.cpp

    src/services/notestore.cpp
    src/services/configmanager.cpp

    src/utils/colorutils.cpp

    src/widgets/tile.cpp
    src/widgets/markdownpreview.cpp
    src/widgets/slidingbuttongroup.cpp
    src/widgets/contextmenu.cpp
    src/widgets/styledcheckbox.cpp
    src/widgets/clampedsplitter.cpp

    src/3rdparty/md4c/md4c.c
    src/3rdparty/md4c/md4c-html.c
    src/3rdparty/md4c/entity.c

    src/windows/mainwindow.cpp
    src/windows/notepadwindow.cpp
    src/windows/settingspanel.cpp
)

set(HEADERS
    src/app/application.h
    src/app/theme.h

    src/services/notestore.h
    src/services/configmanager.h

    src/utils/colorutils.h

    src/widgets/tile.h
    src/widgets/markdownpreview.h
    src/widgets/slidingbuttongroup.h
    src/widgets/contextmenu.h
    src/widgets/styledcheckbox.h
    src/widgets/clampedsplitter.h

    src/windows/mainwindow.h
    src/windows/notepadwindow.h
    src/windows/settingspanel.h
)

add_executable(floral-note ${SOURCES} ${HEADERS})

target_include_directories(floral-note PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(floral-note PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

add_custom_command(TARGET floral-note POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:floral-note>/data
)
