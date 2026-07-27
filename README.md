# topsbot_webviz

[中文文档](README_cn.md)

# Overview

`topsbot_webviz` streams live camera frames from the MES20 board to a web browser for preview.

- **Single channel**: subscribes to an upstream image topic and renders on a Canvas page
- **Detection overlays**: draws boxes from [`topsbot_yolo_det`](../topsbot_yolo_det) `/detections`
- **Multi-channel**: up to four cameras on one page (gateway / multi_instance)

This package handles **Web visualization only**; camera, YOLO, and other upstream nodes run separately—or together via the joint demo launch.

# Bill of Materials

| # | Item | Notes |
| - | ---- | ----- |
| 1 | TOPSBOT MES20 board | Runs `webviz_node` and static HTTP server |
| 2 | Web browser | Same network as the board; open `http://<board-ip>:8000/` |
| 3 | `topsbot_usb_cam` | Publishes image topics (zero-copy or `sensor_msgs`) |
| 4 | `topsbot_yolo_det` (optional) | Publishes `/detections` for box overlays |

# Build and Install

## PC (cross-compile)

Cross-build for MES20 runs inside the `topsbot_cc` Docker environment (x86 host + RISC-V sysroot). Place this repo under `topsbot_cc/topsbot_ws/src/topsbot_webviz` and ensure `topsbot_sysroot` is ready (checked when the Docker container starts).

**1. Enter the cross-build container**

From the `topsbot_cc` repo root:

```bash
sudo topsbot_ws/tools/start_topsbot_cross_build_docker.sh
```

Working directory inside the container: `/mnt/topsbot_cc/topsbot_ws`.

**2. Install sysroot dependencies**

On first build or when CMake reports missing packages (installed into **sysroot**, not the host):

```bash
./tools/sysroot_apt.sh update
./tools/sysroot_apt.sh install \
  libwebsockets-dev libssl-dev \
  libprotobuf-dev protobuf-compiler \
  libopencv-dev \
  tbros-humble-tb-img-msgs tbros-humble-tb-det-msgs
```

| Package | Purpose |
|---------|---------|
| `libwebsockets-dev` | WebSocket server (`ws_server`) |
| `libssl-dev` | OpenSSL (libwebsockets dependency) |
| `libprotobuf-dev` / `protobuf-compiler` | Build/link `topsbot_web.pb.cc`; regenerate with `scripts/regenerate_proto.sh` after editing `proto/` |
| `libopencv-dev` | NV12/RGB → JPEG software fallback |
| `tbros-humble-tb-img-msgs` / `tbros-humble-tb-det-msgs` | Zero-copy image and detection message types |

If TOPSBOT message deb packages are not in your apt source yet, build them from the workspace first:

```bash
./tools/build.sh -s tb_img_msgs,tb_det_msgs
```

**3. Build**

```bash
./tools/build.sh -s topsbot_webviz
```

Optional: NV12 hardware JPEG needs ta-cv in sysroot (`usr/local/lib/libtacocv.so`, etc.). If missing, the build still succeeds and runtime falls back to OpenCV software encoding.

**4. Deploy**

Sync `install/` to the board and `source install/setup.bash`. See the `topsbot_cc` repo for Docker, sysroot, and toolchain details.

## MES20 board (install and source)

**Option A: DEB package (when available)**

```bash
# MES20
source /opt/tbros/humble/setup.bash
sudo apt install -y tbros-humble-topsbot-webviz
```

**Option B: workspace overlay**

```bash
# MES20
source /opt/tbros/humble/setup.bash
source <install>/setup.bash
```

# Usage

Scene configs are full yaml files under `config/`. Pairing with `topsbot_usb_cam` is documented in [`config/README.md`](config/README.md). Quick commands: [`QUICKSTART.md`](QUICKSTART.md).

## Single-channel preview

Use when upstream is already publishing images (e.g. `topsbot_usb_cam` running). Launch: `webviz.launch.py`.

**Default launch**

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py
```

| Item | Default |
|------|---------|
| Scene yaml | `tb_jpeg.yaml` (subscribes `/tbmem_jpeg`) |
| Browser URL | `http://<board-ip>:8000/` |
| HTTP port | `8000` |

For NV12 zero-copy camera pairing:

```bash
# MES20 — upstream: mjpeg2nv12_640x480_zc.yaml
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img.yaml
```

**One command for scene / topic** (include only what you need)

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py \
  params_file:=sensor_image.yaml \
  input_topic:=/cam0/tbmem_img
```

| Launch argument | Purpose |
|-----------------|---------|
| `params_file` | Scene yaml (`input_msg`, default topic, detection flags, …) |
| `input_topic` | Override subscription topic |
| `input_msg` | Override input type |
| `http_port` | Browser HTTP port (default `8000`) |

List all launch arguments: `ros2 launch topsbot_webviz webviz.launch.py -s`

### Pairing with topsbot_usb_cam (640×480)

| usb_cam `params_file` | webviz `params_file` |
|-----------------------|----------------------|
| `mjpeg_640x480_zc.yaml` | `tb_jpeg.yaml` |
| `mjpeg2nv12_640x480_zc.yaml` | `tb_img.yaml` |
| `mjpeg2rgb_640x480_zc.yaml` | `tb_img.yaml` |
| `mjpeg2nv12_640x480_sensor.yaml` | `sensor_image.yaml` |
| `mjpeg_640x480_compressed.yaml` | `sensor_compressed.yaml` |

## Detection overlay mode

Use when YOLO is running and publishing `/detections`. Launch: `webviz.launch.py` with `tb_img_yolo.yaml`.

**Default launch**

```bash
# MES20 — camera + YOLO already running
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img_yolo.yaml
```

**One command adjustments**

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py \
  params_file:=tb_img_yolo.yaml \
  detection_topic:=/detections \
  input_topic:=/tbmem_img
```

| Launch argument | Purpose |
|-----------------|---------|
| `params_file` | `tb_img_yolo.yaml` enables `detection_enabled: true` by default |
| `detection_topic` | Detection topic (default `/detections`) |
| `detection_enabled` | `true` / `false` |

**End-to-end joint test** (camera + YOLO + Web in one launch):

```bash
# MES20
ros2 launch topsbot_yolo_det cam_yolo_webviz_demo.launch.py
```

Browser: `http://<board-ip>:8000/` (webviz uses `tb_img_yolo.yaml` internally).

## Multi-channel preview

Launch: `webviz_multi.launch.py`.
Yaml: `max_channel_count`=4 (max slots), `channel_count`=2 (default selected). Use launch **`channel_count:=4`** for four channels.

```bash
# Default two TbJpeg cameras
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8
ros2 launch topsbot_webviz webviz_multi.launch.py

# Four TbJpeg cameras
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml channel_count:=4 \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8 \
  cam2_video_device:=/dev/video14 cam3_video_device:=/dev/video16
ros2 launch topsbot_webviz webviz_multi.launch.py channel_count:=4

# Four TbMsg* / NV12 cameras
ros2 launch topsbot_webviz webviz_multi.launch.py \
  params_file:=tb_img_multi.yaml

# Four TbMsg* cameras with detections
ros2 launch topsbot_webviz webviz_multi.launch.py \
  params_file:=tb_img_yolo_multi.yaml
```

Browser: `http://<board-ip>:8000/`

# Interface Reference

## ROS topics

This node **subscribes only**; it does not publish ROS image topics. Frames are sent to the browser over WebSocket.

| Direction | Topic | Message type | Notes |
|-----------|-------|--------------|-------|
| Subscribe | `input_topic` in yaml | Depends on `input_msg` | Default `/tbmem_jpeg` or `/tbmem_img` |
| Subscribe (optional) | `detection_topic` | `tb_det_msgs/TbPerceptionTargets` | When `detection_enabled: true` |

## Node parameters

Field dictionary: [`config/params_reference.yaml`](config/params_reference.yaml) (reference only). Grouped summary:

| Group | Key parameters | Notes |
|-------|----------------|-------|
| Image input | `input_msg`, `input_topic`, `tb_img_profile` | Aligned with `topsbot_usb_cam` / `topsbot_yolo_det` |
| Detection overlay | `detection_enabled`, `detection_topic`, `stamp_align` | Boxes and timestamp alignment |
| Channel / deployment | `channel_count`, `channel_id`, `channel_name`, `deployment` | Single / 4ch gateway / multi_instance |
| Web transport | `ws_base_port`, `gateway_ws_port`, `jpeg_quality`, `enable_tacv_jpeg`, `queue_max` | WebSocket ports; NV12 uses serialized ta-cv hardware encoding first |
| HTTP | launch `http_port` | Static page port (default `8000`) |

Built-in single-channel `params_file` files:

| `params_file` | `input_msg` | Typical upstream |
|---------------|-------------|------------------|
| `tb_jpeg.yaml` | `tb_jpeg` | `mjpeg_640x480_zc.yaml` (**launch default**) |
| `tb_img.yaml` | `tb_img` | `mjpeg2nv12_640x480_zc.yaml` |
| `sensor_image.yaml` | `sensor_image` | `mjpeg2nv12_640x480_sensor.yaml` |
| `sensor_compressed.yaml` | `sensor_compressed` | `mjpeg_640x480_compressed.yaml` |
| `tb_img_yolo.yaml` | `tb_img` + detection | camera + `topsbot_yolo_det` |

Merge order: `params_file` yaml → launch overrides (non-empty only). See `config/README.md`.

## Launch files

| Launch | Purpose |
|--------|---------|
| `webviz.launch.py` | Single-channel preview |
| `webviz_multi.launch.py` | Multi-channel (yaml = max channels; `channel_count:=N` selects) |

List arguments: `ros2 launch topsbot_webviz <launch> -s`

---

## License

Apache License 2.0 — see [LICENSE](./LICENSE) when provided in the repository.
