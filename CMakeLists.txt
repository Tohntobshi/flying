cmake_minimum_required(VERSION 2.8)
project( Flying )

set(
  TARGETS
    src/main.cpp
    src/flightController.cpp
    src/connection.cpp
    src/debugSender.cpp
    src/packetsSender.cpp
    src/packetsReceiver.cpp
    src/packetsTransmitter.cpp
    src/separateLoop.cpp
    src/controlsReceiver.cpp
    # src/frameCapturer.cpp
    src/separateLoop.cpp
    # src/frameSender.cpp
)

include_directories(libs/glm/include)
include_directories(libs/rapidjson/include)
include_directories(libs/bmp280/include)

add_executable( flying ${TARGETS} )

add_subdirectory( libs/bmp280 )

target_link_libraries(flying bmp280)
target_link_libraries(flying bluetooth)
target_link_libraries(flying pigpio)
target_link_libraries(flying pthread)

# target_link_libraries(flying avcodec)
# target_link_libraries(flying avutil)
# target_link_libraries(flying swscale)
# target_link_libraries(flying avformat)

