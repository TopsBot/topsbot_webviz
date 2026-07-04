# topsbot_webviz 快速入门

完整说明见 `README_cn.md`。

## 1. 单路预览

```bash
# MES20 — 默认（/tbmem_jpeg）
ros2 launch topsbot_webviz webviz.launch.py

# MES20 — NV12 零拷贝（/tbmem_img）
ros2 launch topsbot_webviz webviz.launch.py params_file:=tbmem_img_zc.yaml

# MES20 — 改话题
ros2 launch topsbot_webviz webviz.launch.py \
  params_file:=tbmem_img_zc.yaml input_topic:=/cam0/tbmem_img
```

浏览器 `http://<板卡IP>:8000/`。上游相机需先启动。

## 2. 检测框叠加

```bash
# MES20
ros2 launch topsbot_webviz webviz.launch.py params_file:=tbmem_img_yolo.yaml
```

或一次启动完整链路：

```bash
ros2 launch topsbot_yolo_det cam_yolo_webviz_demo.launch.py
```

## 3. 四路预览

```bash
# MES20
ros2 launch topsbot_webviz webviz_4ch.launch.py params_file:=4ch_gateway.yaml
```

浏览器 `http://<板卡IP>:8000/?mode=gateway&max_channels=4`

## 4. 查看 launch 参数

```bash
ros2 launch topsbot_webviz webviz.launch.py -s
ros2 launch topsbot_webviz webviz_4ch.launch.py -s
```

## 5. 配置与成对关系

- 场景 yaml：`config/README.md`
- 参数字典：`config/params_reference.yaml`
- 与 `topsbot_usb_cam` 成对表见 `README_cn.md`
