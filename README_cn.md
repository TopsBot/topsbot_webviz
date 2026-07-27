# topsbot_webviz

[English](README.md)

# 功能介绍

`topsbot_webviz` 在 MES20 开发板上把相机画面推到浏览器里实时预览。

- **单路预览**：订阅上游图像话题，在网页 Canvas 上显示
- **检测框叠加**：配合 [`topsbot_yolo_det`](../topsbot_yolo_det) 的 `/detections`，在画面上画框
- **多路预览**：最多四路相机同屏（gateway / multi_instance）

本包只负责 **Web 可视化**；相机、YOLO 等上游节点需单独启动，或由联合 demo 一并拉起。

# 物料清单

| 序号 | 名称 | 说明 |
| ---- | ---- | ---- |
| 1 | TOPSBOT MES20 板卡 | 板端运行 `webviz_node` 与静态 HTTP 服务 |
| 2 | 浏览器 | 与板卡同网段，访问 `http://<板卡IP>:8000/` |
| 3 | `topsbot_usb_cam` | 提供图像话题（零拷贝或 `sensor_msgs`） |
| 4 | `topsbot_yolo_det`（可选） | 提供 `/detections`，用于检测框叠加 |

# 编译与安装

## PC 端（交叉编译）

面向 MES20 的交叉编译在 `topsbot_cc` Docker 环境中完成（x86 主机 + RISC-V sysroot）。将本仓库置于 `topsbot_cc/topsbot_ws/src/topsbot_webviz`，并确保 `topsbot_sysroot` 已就绪（启动 Docker 时会自动检查）。

**1. 进入交叉编译容器**

在 `topsbot_cc` 仓库根目录执行：

```bash
sudo topsbot_ws/tools/start_topsbot_cross_build_docker.sh
```

容器内工作目录为 `/mnt/topsbot_cc/topsbot_ws`。

**2. 安装 sysroot 依赖**

首次编译或 CMake 报缺包时，在容器内执行（依赖安装到 **sysroot**，不是宿主机）：

```bash
./tools/sysroot_apt.sh update
./tools/sysroot_apt.sh install \
  libwebsockets-dev libssl-dev \
  libprotobuf-dev protobuf-compiler \
  libopencv-dev \
  tbros-humble-tb-img-msgs tbros-humble-tb-det-msgs
```

| 包 | 用途 |
|----|------|
| `libwebsockets-dev` | WebSocket 服务（`ws_server`） |
| `libssl-dev` | OpenSSL（libwebsockets 依赖） |
| `libprotobuf-dev` / `protobuf-compiler` | 编译与链接 `topsbot_web.pb.cc`；修改 `proto/` 后可用 `scripts/regenerate_proto.sh` 重新生成 |
| `libopencv-dev` | NV12/RGB → JPEG 软编码回退路径 |
| `tbros-humble-tb-img-msgs` / `tbros-humble-tb-det-msgs` | 订阅零拷贝图像与检测框消息 |

若 apt 源中尚无 TOPSBOT 消息包，可先编译工作空间内的消息包：

```bash
./tools/build.sh -s tb_img_msgs,tb_det_msgs
```

**3. 编译**

```bash
./tools/build.sh -s topsbot_webviz
```

可选：开启 NV12 硬件 JPEG 编码需 sysroot 内已有 ta-cv（`usr/local/lib/libtacocv.so` 等）；缺省时自动走 OpenCV 软编码，不影响编译。

**4. 部署**

将 `install/` 同步到板卡后 `source install/setup.bash`。Docker、sysroot 与工具链说明见 `topsbot_cc` 仓库文档。

## MES20 板端（安装与加载）

**方式 A：DEB 包（源可用时）**

```bash
# MES20
source /opt/tbros/humble/setup.bash
sudo apt install -y tbros-humble-topsbot-webviz
```

**方式 B：加载工作空间**

```bash
# MES20
source /opt/tbros/humble/setup.bash
source <install>/setup.bash
```

# 使用方式

场景配置为全量 yaml，位于 `config/`。成对关系见 [`config/README.md`](config/README.md)。快速命令见 [`QUICKSTART.md`](QUICKSTART.md)。

## 单路预览模式

适用：上游已在发布图像（如已启动 `topsbot_usb_cam`）。Launch：`webviz.launch.py`。

**默认启动**

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py
```

| 项 | 默认值 |
|----|--------|
| 场景 yaml | `tb_jpeg.yaml`（订阅 `/tbmem_jpeg`） |
| 浏览器地址 | `http://<板卡IP>:8000/` |
| HTTP 端口 | `8000` |

与 NV12 零拷贝相机成对使用时，改用：

```bash
# MES20 — 上游：mjpeg2nv12_640x480_zc.yaml
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img.yaml
```

**一条命令改场景 / 话题**（只写需要改的即可）

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py \
  params_file:=sensor_image.yaml \
  input_topic:=/cam0/tbmem_img
```

| launch 参数 | 作用 |
|-------------|------|
| `params_file` | 场景 yaml（`input_msg`、默认话题、检测开关等） |
| `input_topic` | 覆盖订阅话题 |
| `input_msg` | 覆盖输入类型 |
| `http_port` | 浏览器 HTTP 端口（默认 `8000`） |

查看全部 launch 参数：`ros2 launch topsbot_webviz webviz.launch.py -s`

### 与 topsbot_usb_cam 成对（640×480）

| usb_cam `params_file` | webviz `params_file` |
|-----------------------|----------------------|
| `mjpeg_640x480_zc.yaml` | `tb_jpeg.yaml` |
| `mjpeg2nv12_640x480_zc.yaml` | `tb_img.yaml` |
| `mjpeg2rgb_640x480_zc.yaml` | `tb_img.yaml` |
| `mjpeg2nv12_640x480_sensor.yaml` | `sensor_image.yaml` |
| `mjpeg_640x480_compressed.yaml` | `sensor_compressed.yaml` |

## 检测框叠加模式

适用：已启动 YOLO 并发布 `/detections`。Launch：同上 `webviz.launch.py`，使用 `tb_img_yolo.yaml`。

**默认启动**

```bash
# MES20 — 上游相机 + YOLO 已运行
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img_yolo.yaml
```

**一条命令调整**

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py \
  params_file:=tb_img_yolo.yaml \
  detection_topic:=/detections \
  input_topic:=/tbmem_img
```

| launch 参数 | 作用 |
|-------------|------|
| `params_file` | `tb_img_yolo.yaml` 已默认开启 `detection_enabled: true` |
| `detection_topic` | 检测话题（默认 `/detections`） |
| `detection_enabled` | `true` / `false` |

**端到端联合测试**（相机 + YOLO + Web 一次启动）：

```bash
# MES20
ros2 launch topsbot_yolo_det cam_yolo_webviz_demo.launch.py
```

浏览器 `http://<板卡IP>:8000/`，webviz 使用内置 `tb_img_yolo.yaml`。

## 多路预览

适用：多路 USB 相机同屏。Launch：`webviz_multi.launch.py`。
**yaml：`max_channel_count`=4（最大路数），`channel_count`=2（默认启用路数）**；四路需 launch **`channel_count:=4`**。

```bash
# 默认两路 TbJpeg
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8
ros2 launch topsbot_webviz webviz_multi.launch.py

# 四路 TbJpeg
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml channel_count:=4 \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8 \
  cam2_video_device:=/dev/video14 cam3_video_device:=/dev/video16
ros2 launch topsbot_webviz webviz_multi.launch.py channel_count:=4

# 四路 TbMsg* / NV12
ros2 launch topsbot_webviz webviz_multi.launch.py \
  params_file:=tb_img_multi.yaml

# 四路 TbMsg* + 检测框
ros2 launch topsbot_webviz webviz_multi.launch.py \
  params_file:=tb_img_yolo_multi.yaml
```

浏览器：`http://<板卡IP>:8000/`

# 接口说明

## ROS 话题

本节点**仅订阅**，不发布 ROS 图像话题；画面经 WebSocket 推送到浏览器。

| 方向 | 话题 | 消息类型 | 说明 |
|------|------|----------|------|
| 订阅 | yaml 中 `input_topic` | 由 `input_msg` 决定 | 默认 `/tbmem_jpeg` 或 `/tbmem_img` |
| 订阅（可选） | `detection_topic` | `tb_det_msgs/TbPerceptionTargets` | `detection_enabled: true` 时 |

## 节点参数

字段字典见 [`config/params_reference.yaml`](config/params_reference.yaml)（不自动加载）。按功能分组如下：

| 分组 | 主要参数 | 说明 |
|------|----------|------|
| 图像订阅 | `input_msg`、`input_topic`、`tb_img_profile` | 与 `topsbot_usb_cam` / `topsbot_yolo_det` 约定一致 |
| 检测叠加 | `detection_enabled`、`detection_topic`、`stamp_align` | 检测框与时间戳对齐 |
| 通道与部署 | `channel_count`、`channel_id`、`channel_name`、`deployment` | 单路 / 四路 gateway / multi_instance |
| Web 传输 | `ws_base_port`、`gateway_ws_port`、`jpeg_quality`、`enable_tacv_jpeg`、`queue_max` | WebSocket 端口；NV12 默认 ta-cv 硬编优先，多路跨进程串行化 |
| HTTP | launch `http_port` | 静态页面端口（默认 `8000`） |

内置单路 `params_file`：

| `params_file` | `input_msg` | 典型上游 |
|---------------|-------------|----------|
| `tb_jpeg.yaml` | `tb_jpeg` | `mjpeg_640x480_zc.yaml`（**launch 默认**） |
| `tb_img.yaml` | `tb_img` | `mjpeg2nv12_640x480_zc.yaml` |
| `sensor_image.yaml` | `sensor_image` | `mjpeg2nv12_640x480_sensor.yaml` |
| `sensor_compressed.yaml` | `sensor_compressed` | `mjpeg_640x480_compressed.yaml` |
| `tb_img_yolo.yaml` | `tb_img` + 检测 | 相机 + `topsbot_yolo_det` |

参数合并顺序：`params_file` yaml → launch 覆盖项（非空才生效）。详见 `config/README.md`。

## Launch 文件

| Launch | 用途 |
|--------|------|
| `webviz.launch.py` | 单路预览 |
| `webviz_multi.launch.py` | 多路预览（yaml 定最大路数，`channel_count` 选实际路数） |

各 launch 参数：`ros2 launch topsbot_webviz <launch名> -s`

---

## 许可

Apache License 2.0 — 见 [LICENSE](./LICENSE)（若仓库提供）。
