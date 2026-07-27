# topsbot_webviz 快速入门

## tb_jpeg vs tb_img（一眼分清）

| | tb_jpeg | tb_img |
|--|---------|--------|
| 话题示例 | `/tbmem_jpeg` | `/tbmem_img` |
| 内容 | JPEG 码流 | NV12/RGB 像素 |
| usb_cam | `mjpeg_*_zc` | `mjpeg2nv12_*_zc` / `mjpeg2rgb_*_zc` |

## 单路

```bash
ros2 launch topsbot_webviz webviz.launch.py
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img.yaml
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img_yolo.yaml
```

## 多路（默认 2 路；四路 `channel_count:=4`）

```bash
# 默认两路相机 + webviz
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8
ros2 launch topsbot_webviz webviz_multi.launch.py

# 四路
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml channel_count:=4 \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8 \
  cam2_video_device:=/dev/video14 cam3_video_device:=/dev/video16
ros2 launch topsbot_webviz webviz_multi.launch.py channel_count:=4

# TbMsg* / NV12 多路
ros2 launch topsbot_webviz webviz_multi.launch.py \
  params_file:=tb_img_multi.yaml

# TbMsg* 多路 + 每路 /camN/detections
ros2 launch topsbot_webviz webviz_multi.launch.py \
  params_file:=tb_img_yolo_multi.yaml
```

所有配置默认 `enable_tacv_jpeg: true`。NV12 使用 ta-cv 硬编优先；
多路节点会跨进程串行访问板端单一 JPEG 硬编资源。

浏览器：`http://<板卡IP>:8000/`

详见 `config/README.md`。
