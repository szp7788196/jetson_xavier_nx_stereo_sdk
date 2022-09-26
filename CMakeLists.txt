cmake_minimum_required(VERSION 3.10)

project(stereo)

set(CMAKE_BUILD_TYPE Release)

# set(JPEG_DIR /usr/local/lib)

############################################################
# Create a library
############################################################

find_package(OpenCV REQUIRED)

#Generate the static library from the library sources
add_library(stereo SHARED
    cmd_parse/cmd_parse.c
    cssc132/cssc132.c
    handler/handler.c
    led/led.c
    stereo/stereo.c
    mpu9250/mpu9250.c
    net/net.c
    serial/serial.c
    sync/sync.c
    sync_module/sync_module.c
    ub482/ub482.c
    # ui3240/ui3240.c
    m3st130h/m3st130h.c
    fifo/ringbuf.c
    can/can.c
    odb2/odb2.c
)

target_link_libraries(stereo
    m
    # ${JPEG_DIR}/libjpeg.a
    png
    pthread
    # ueye_api
    dvp
    hzd
)

# target_include_directories(stereo
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/serial
#         ${PROJECT_SOURCE_DIR}/net
#         ${PROJECT_SOURCE_DIR}/ub482
#         ${PROJECT_SOURCE_DIR}/stereo
#         ${PROJECT_SOURCE_DIR}/cmd_parse
#         ${PROJECT_SOURCE_DIR}/mpu9250
#         ${PROJECT_SOURCE_DIR}/ui3240
#         ${PROJECT_SOURCE_DIR}/cssc132
#         ${PROJECT_SOURCE_DIR}/sync_module
#         ${PROJECT_SOURCE_DIR}/led
#         ${PROJECT_SOURCE_DIR}/sync
#         ${PROJECT_SOURCE_DIR}/interface
# )

include_directories(
    # ${UEYE_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/cmd_parse
    ${PROJECT_SOURCE_DIR}/cssc132
    ${PROJECT_SOURCE_DIR}/handler
    ${PROJECT_SOURCE_DIR}/led
    ${PROJECT_SOURCE_DIR}/stereo
    ${PROJECT_SOURCE_DIR}/mpu9250
    ${PROJECT_SOURCE_DIR}/net
    ${PROJECT_SOURCE_DIR}/serial
    ${PROJECT_SOURCE_DIR}/sync
    ${PROJECT_SOURCE_DIR}/sync_module
    ${PROJECT_SOURCE_DIR}/ub482
    # ${PROJECT_SOURCE_DIR}/ui3240
    ${PROJECT_SOURCE_DIR}/m3st130h
    ${PROJECT_SOURCE_DIR}/fifo
    ${PROJECT_SOURCE_DIR}/can
    ${PROJECT_SOURCE_DIR}/odb2
)

############################################################
# Create an executable
############################################################

# Add an executable with the above sources
add_executable(sdk
    main.cpp
#    main_read.c
)

# link the new hello_library target with the stereo target
target_link_libraries(sdk
    PRIVATE
        stereo
        ${OpenCV_LIBRARIES}
)
