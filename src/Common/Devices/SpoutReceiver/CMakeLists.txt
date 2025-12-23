cmake_minimum_required(VERSION 3.10)

project(SpoutReceiver LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network OpenGL)

set(SpoutLibrary_LIBRARIES "${DEPENDS_DIR}/Spout2-2.007.017/SPOUTSDK/SpoutLibrary/x64/Release/SpoutLibrary.lib")
INCLUDE_DIRECTORIES("${DEPENDS_DIR}/Spout2-2.007.017/SPOUTSDK/SpoutLibrary")
message("--Found Spout SDK ${SpoutLibrary_LIBRARIES}")

set(SpoutReceiver_sources
    SpoutReceiver.cpp
    SpoutReceiver.h
    SpoutReceiverWorker.cpp
    SpoutReceiverWorker.h
)

add_library(SpoutReceiver SHARED ${SpoutReceiver_sources})

target_link_libraries(SpoutReceiver PUBLIC
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::OpenGL
    ${OpenCV_LIBRARIES}
)
target_link_libraries(SpoutReceiver PRIVATE ${SpoutLibrary_LIBRARIES})

target_compile_definitions(SpoutReceiver PRIVATE SPOUTRECEIVER_LIBRARY)

