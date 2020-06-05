cmake_minimum_required(VERSION 2.8)
project( Flying )

set(
  TARGETS
    src/main.cpp
    src/flightController.cpp
    src/connection.cpp
    src/debugSender.cpp
    src/packetsSender.cpp
    src/separateLoop.cpp
    src/controlsReceiver.cpp
    src/frameCapturer.cpp
    src/separateLoop.cpp
    src/frameSender.cpp
)

include_directories(libs/glm/include)
include_directories(libs/rapidjson/include)

add_executable( flying ${TARGETS} )

target_link_libraries(flying bluetooth)

target_link_libraries(flying pigpio)

target_link_libraries(flying pthread)

target_link_libraries(flying avcodec)
target_link_libraries(flying avutil)
target_link_libraries(flying swscale)
target_link_libraries(flying avformat)

target_link_libraries(flying ncurses)
