cmake_minimum_required(VERSION 3.16)
project(SoundBoard)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Network)

qt_standard_project_setup()

add_executable(SoundBoard
    main.cpp
    MainWindow.cpp
    MainWindow.h
    PadPage.cpp
    PadPage.h
    Sound.cpp
    Sound.h
    SoundManager.cpp
    SoundManager.h
    HostSession.cpp
    HostSession.h
    JoinSession.cpp
    JoinSession.h
)

qt_add_resources(RESOURCES resources.qrc)
target_sources(SoundBoard PRIVATE ${RESOURCES})


target_link_libraries(SoundBoard PRIVATE Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Network)

