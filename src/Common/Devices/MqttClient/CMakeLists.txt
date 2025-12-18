cmake_minimum_required(VERSION 3.10)

project(MqttClient LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

# QMQTT path
set(QMQTT_INSTALL_DIR "${DEPENDS_DIR}/qmqtt/install")
include_directories("${QMQTT_INSTALL_DIR}/include")
link_directories("${QMQTT_INSTALL_DIR}/lib")

set(MqttClient_sources
    MqttClient.cpp
    MqttClient.h
    MqttClientWorker.cpp
    MqttClientWorker.h
)

add_library(MqttClient SHARED ${MqttClient_sources})

target_link_libraries(MqttClient PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Network
)
target_link_libraries(MqttClient PRIVATE qmqtt)

target_compile_definitions(MqttClient PRIVATE MQTTCLIENT_LIBRARY)

# if(WIN32)
#     add_custom_command(TARGET MqttClient POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         "${QMQTT_INSTALL_DIR}/bin/qmqtt.dll"
#         "$<TARGET_FILE_DIR:MqttClient>/qmqtt.dll"
#     )
# endif()
