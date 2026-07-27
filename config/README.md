# config/ 目录说明

## 命名：按输入类型（不含路数）

| 文件 | 含义 | Launch |
|------|------|--------|
| `tb_jpeg.yaml` | TbJpegFrame 单路 `/tbmem_jpeg` | `webviz.launch.py`（**默认**） |
| `tb_img.yaml` | TbMsg* 单路 `/tbmem_img` | `webviz.launch.py` |
| `tb_img_yolo.yaml` | TbMsg* + 检测框 | `webviz.launch.py` |
| `sensor_image.yaml` | `sensor_msgs/Image` | `webviz.launch.py` |
| `sensor_compressed.yaml` | `sensor_msgs/CompressedImage` | `webviz.launch.py` |
| `tb_jpeg_multi.yaml` | TbJpeg 多路 `/camN/tbmem_jpeg` | `webviz_multi.launch.py`（**默认**） |
| `tb_img_multi.yaml` | TbMsg* 多路 `/camN/tbmem_img` | `webviz_multi.launch.py` |
| `tb_img_yolo_multi.yaml` | TbMsg* 多路 + `/camN/detections` 检测框 | `webviz_multi.launch.py` |
| `sensor_image_multi.yaml` | `sensor_msgs/Image` 多路 `/camN/image_raw` | `webviz_multi.launch.py` |
| `sensor_compressed_multi.yaml` | `CompressedImage` 多路 `/camN/image_raw/compressed` | `webviz_multi.launch.py` |

多路 yaml 的 `deployment` 默认是 `multi_instance`；要临时使用单节点 gateway，
追加 `deployment:=gateway` 覆盖，无需更换 yaml。

## 路数参数（写在 yaml 里）

| 参数 | 含义 |
|------|------|
| `max_channel_count` | **最大**路数（多路 yaml 固定为 4，含 `ch0_`…`ch3_`） |
| `channel_count` | **默认启用**路数（多路 yaml 默认为 **2**） |

Launch 覆盖启用路数（四路需显式指定）：

```bash
ros2 launch topsbot_webviz webviz_multi.launch.py
# 四路全开
ros2 launch topsbot_webviz webviz_multi.launch.py channel_count:=4
```

## tb_jpeg vs tb_img

| | `tb_jpeg` | `tb_img` |
|--|-----------|----------|
| 消息 | `TbJpegFrame` | `TbMsg480P` / `540P` / `1080P` |
| 内容 | 已压缩的 JPEG 码流 | 原始像素（NV12/RGB 等，看 `encoding`） |
| 典型上游 | usb_cam `mjpeg` + 零拷贝 | usb_cam `mjpeg2nv12` / `mjpeg2rgb` 零拷贝 |
| webviz | 几乎直接推浏览器 | 先 JPEG 编码再推浏览器 |

## JPEG 编码器

所有类型配置都显式设置 `enable_tacv_jpeg: true`。对 NV12 原始图，
webviz 优先使用 ta-cv 硬件 JPEG 编码；多路/多进程调用通过跨进程锁串行访问
板端单一硬编资源，硬编真正失败时才回退 OpenCV。`tb_jpeg` 和
`sensor_compressed` 已经是压缩码流，不会触发重新编码；RGB 输入仍由
OpenCV 编码。

## 示例

```bash
# 单路
ros2 launch topsbot_webviz webviz.launch.py
ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img.yaml

# 默认两路
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8
ros2 launch topsbot_webviz webviz_multi.launch.py

# 四路全开
ros2 launch topsbot_usb_cam usb_cam_multi.launch.py \
  params_file:=mjpeg_640x480_zc.yaml channel_count:=4 \
  cam0_video_device:=/dev/video6 cam1_video_device:=/dev/video8 \
  cam2_video_device:=/dev/video14 cam3_video_device:=/dev/video16
ros2 launch topsbot_webviz webviz_multi.launch.py channel_count:=4
```

浏览器：`http://<板卡IP>:8000/`
