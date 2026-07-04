# config/ 目录说明

## 文件命名

单路场景（全量 yaml）：

```
{订阅场景}_{上游简述}.yaml
```

| 文件 | input_msg | input_topic | 配合 topsbot_usb_cam |
|------|-----------|-------------|----------------------|
| `tbmem_jpeg_zc.yaml` | tb_jpeg | `/tbmem_jpeg` | `mjpeg_640x480_zc.yaml` |
| `tbmem_img_zc.yaml` | tb_img | `/tbmem_img` | `mjpeg2nv12_640x480_zc.yaml` 等 |
| `image_raw_sensor.yaml` | sensor_image | `/image_raw` | `mjpeg2nv12_640x480_sensor.yaml` |
| `image_raw_compressed.yaml` | sensor_compressed | `/image_raw/compressed` | `mjpeg_640x480_compressed.yaml` |
| `tbmem_img_yolo.yaml` | tb_img + detection | `/tbmem_img` | 同上 + yolo |

多路：`4ch_gateway.yaml`、`4ch_multi.yaml`（`channel_count: 4` + `ch0_*` …）

## 图像订阅参数（TOPSBOT 软性约定）

| 参数 | 说明 |
|------|------|
| `input_topic` | 订阅话题 |
| `input_msg` | `tb_jpeg` / `tb_img` / `sensor_image` / `sensor_compressed` |
| `tb_img_profile` | **仅** `input_msg=tb_img`：`480p` / `540p` / `1080p`（与上游分辨率成对写在 yaml） |

`tb_img_profile` 表示 `TbMsg480P` / `TbMsg540P` / `TbMsg1080P` 消息类型，不是分辨率名。格式由消息内 `encoding` 字段决定。

旧参数 `input_mode` / `input_compressed` 仍可读（启动 WARN），会映射到 `input_msg`。

## 参数合并顺序（后者覆盖前者）

1. `params_file` — 上表全量 yaml
2. launch 覆盖 — `input_topic`、`input_msg`、`tb_img_profile`、检测/通道/WS 等

```bash
ros2 launch topsbot_webviz webviz.launch.py params_file:=tbmem_img_zc.yaml
ros2 launch topsbot_webviz webviz.launch.py -s
```

## 其他文件

| 文件 | 说明 |
|------|------|
| `params_reference.yaml` | 参数速查（**不自动加载**） |
