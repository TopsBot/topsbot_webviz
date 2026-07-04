cmake_minimum_required(VERSION 3.8)
project(topsbot_webviz)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tb_img_msgs REQUIRED)
find_package(tb_det_msgs REQUIRED)
find_package(OpenCV 4 REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)

include(cmake/target_deps.cmake)

set(SOURCES
  src/main.cpp
  src/webviz_node.cpp
  src/channel_pipeline.cpp
  src/core/types.cpp
  src/core/config.cpp
  src/core/match_queue.cpp
  src/adapter/overlay_adapter.cpp
  src/encoder/nv12_to_jpeg.cpp
  src/transport/ws_server.cpp
  src/protobuf/topsbot_web.pb.cc
  src/utils/threads/threadpool.cpp
  src/utils/time_helper.cpp
)

if(TOPSBOT_WEBVIZ_HAS_TACV)
  list(APPEND SOURCES
    src/encoder/dmabuf_allocator.cpp
    src/encoder/ta_image.cpp)
endif()

add_executable(webviz_node ${SOURCES})

target_include_directories(webviz_node PRIVATE
  include
  include/utils
  src/protobuf
)

ament_target_dependencies(webviz_node
  rclcpp
  sensor_msgs
)

topsbot_webviz_apply_deps(webviz_node)

target_link_libraries(webviz_node
  ${OpenCV_LIBS}
  tb_img_msgs::tb_img_msgs__rosidl_generator_cpp
  tb_img_msgs::tb_img_msgs__rosidl_typesupport_cpp
  tb_det_msgs::tb_det_msgs__rosidl_generator_cpp
  tb_det_msgs::tb_det_msgs__rosidl_typesupport_cpp
)

install(TARGETS webviz_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY config launch web proto
  DESTINATION share/${PROJECT_NAME})

install(FILES QUICKSTART.md DESTINATION share/${PROJECT_NAME})

install(PROGRAMS scripts/regenerate_proto.sh
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_rate_limiter test/test_rate_limiter.cpp
    src/core/rate_limiter.cpp
    src/core/config.cpp
    src/core/types.cpp)
  target_include_directories(test_rate_limiter PRIVATE include src/protobuf)
  topsbot_webviz_apply_deps(test_rate_limiter)
  ament_target_dependencies(test_rate_limiter rclcpp)
  target_include_directories(test_rate_limiter PRIVATE
    ${tb_det_msgs_DIR}/../../../include/tb_det_msgs)
  target_link_libraries(test_rate_limiter
    tb_det_msgs::tb_det_msgs__rosidl_generator_cpp)

  ament_add_gtest(test_match_queue test/test_match_queue.cpp
    src/core/match_queue.cpp
    src/core/types.cpp)
  target_include_directories(test_match_queue PRIVATE include)
  ament_target_dependencies(test_match_queue)
  target_include_directories(test_match_queue PRIVATE
    ${tb_det_msgs_DIR}/../../../include/tb_det_msgs)
  target_link_libraries(test_match_queue
    tb_det_msgs::tb_det_msgs__rosidl_generator_cpp)

  ament_add_gtest(test_overlay_adapter test/test_overlay_adapter.cpp
    src/adapter/overlay_adapter.cpp
    src/core/types.cpp)
  target_include_directories(test_overlay_adapter PRIVATE include src/protobuf)
  topsbot_webviz_apply_deps(test_overlay_adapter)
  ament_target_dependencies(test_overlay_adapter)
  target_include_directories(test_overlay_adapter PRIVATE
    ${tb_det_msgs_DIR}/../../../include/tb_det_msgs)
  target_link_libraries(test_overlay_adapter
    tb_det_msgs::tb_det_msgs__rosidl_generator_cpp)
endif()

ament_package()
