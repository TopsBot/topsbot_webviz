# MES20 / riscv64 target dependencies (protobuf, libwebsockets, ta-cv + dmabuf).

if(CMAKE_SYSROOT)
  find_library(TOPSBOT_WEBVIZ_PROTOBUF_LIB protobuf REQUIRED)
  find_path(TOPSBOT_WEBVIZ_LWS_INCLUDE libwebsockets.h
    PATHS "${CMAKE_SYSROOT}/usr/include" NO_DEFAULT_PATH)
  find_library(TOPSBOT_WEBVIZ_LWS_LIB websockets
    PATHS
      "${CMAKE_SYSROOT}/usr/lib/riscv64-linux-gnu"
      "${CMAKE_SYSROOT}/usr/lib"
    NO_DEFAULT_PATH)
else()
  find_package(Protobuf REQUIRED)
  set(TOPSBOT_WEBVIZ_PROTOBUF_LIB protobuf::libprotobuf)
  find_path(TOPSBOT_WEBVIZ_LWS_INCLUDE libwebsockets.h)
  find_library(TOPSBOT_WEBVIZ_LWS_LIB websockets)
endif()

if(NOT TOPSBOT_WEBVIZ_LWS_INCLUDE OR NOT TOPSBOT_WEBVIZ_LWS_LIB)
  message(FATAL_ERROR "libwebsockets not found; install libwebsockets-dev in sysroot")
endif()

message(STATUS "topsbot_webviz: libwebsockets=${TOPSBOT_WEBVIZ_LWS_LIB}")

if(CMAKE_SYSROOT)
  set(TOPSBOT_TACV_ROOT "${CMAKE_SYSROOT}/usr/local")
  find_path(TOPSBOT_TACV_INCLUDE ta_cv_api_ext_c.h
    PATHS "${TOPSBOT_TACV_ROOT}/include" NO_DEFAULT_PATH)
  find_library(TOPSBOT_TACV_LIB tacocv PATHS "${TOPSBOT_TACV_ROOT}/lib" NO_DEFAULT_PATH)
  if(TOPSBOT_TACV_INCLUDE AND TOPSBOT_TACV_LIB)
    set(TOPSBOT_WEBVIZ_HAS_TACV TRUE)
    message(STATUS "topsbot_webviz: ta-cv ${TOPSBOT_TACV_LIB}")
  endif()
endif()

message(STATUS "topsbot_webviz: protobuf=${TOPSBOT_WEBVIZ_PROTOBUF_LIB}")

function(topsbot_webviz_apply_deps target)
  if(TARGET ${TOPSBOT_WEBVIZ_PROTOBUF_LIB})
    target_link_libraries(${target} ${TOPSBOT_WEBVIZ_PROTOBUF_LIB})
  else()
    target_link_libraries(${target} ${TOPSBOT_WEBVIZ_PROTOBUF_LIB})
  endif()
  target_include_directories(${target} PRIVATE ${TOPSBOT_WEBVIZ_LWS_INCLUDE})
  target_link_libraries(${target} ${TOPSBOT_WEBVIZ_LWS_LIB})
  if(TOPSBOT_WEBVIZ_HAS_TACV)
    target_compile_definitions(${target} PRIVATE TOPSBOT_WEBVIZ_HAS_TACV)
    target_include_directories(${target} PRIVATE ${TOPSBOT_TACV_INCLUDE})
    target_link_directories(${target} PRIVATE "${TOPSBOT_TACV_ROOT}/lib")
    target_link_libraries(${target}
      ${TOPSBOT_TACV_LIB}
      dmabufheap
      tacosys)
  endif()
endfunction()
