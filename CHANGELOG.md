# Changelog

## 0.2.0 (2026-07-24)

### Added

- `enable_tacv_jpeg` config and launch override; startup log reports `nv12_jpeg_encoder=...`.
- Process-wide flock on `/tmp/topsbot_webviz_tacv_jpeg.lock` so multiple `webviz_node` instances serialize ta-cv JPEG hardware encode.
- Browser keypoints overlay: `OverlayAdapter` maps `TbTarget.points` (`TbPoint` / `body_kps`/`hand_kps`/`face_kps`) into protobuf `PointSet`; frontend draws skeletons (hand/pose connections) when the keypoints layer is enabled.

### Changed

- Multi-channel yamls: `max_channel_count: 4`, default `channel_count: 2`; explicit `enable_tacv_jpeg: true` where applicable.
- Docs and `config/README.md` aligned with type-based yaml names and channel defaults.
- Enable the UI keypoints toggle (was disabled); draw overlay keypoints alongside boxes.

## 0.1.0

- Initial C++ webviz_node + browser frontend.

### Changed

- Launch: `webviz.launch.py` (1ch) / `webviz_multi.launch.py` (multi; `channel_count` selects 1…yaml max).
- Config naming by type (`tb_jpeg.yaml`, `tb_img_multi.yaml`, …); `max_channel_count` + `channel_count` in yaml.
- Config: remove `config/presets/` and `webviz_common.yaml`; use full flat `config/*.yaml` only.
- Launch: `preset` → `params_file`; align merge order with TOPSBOT launch/config spec.
- Subscription params: `input_topic`, `input_msg`, `tb_img_profile` (aligned with topsbot_cv / topsbot_yolo_det).
- Remove `image_type` / `image_topic`; legacy `input_mode` / `input_compressed` still accepted with WARN.
- Fix MatchQueue `strict` blocking image-only preview when `detection_enabled` is false.
- Docs: `QUICKSTART.md`, `config/README.md`, `config/params_reference.yaml`.

### Removed

- `config/presets/`, `config/webviz_common.yaml`, `config/4ch.yaml` alias.
- Launch `webviz.launch.py` / `webviz_4ch.launch.py` (replaced by `webviz` / `webviz_multi`).
- Launch argument `preset`; ROS params `image_type`, `image_topic`.
- `scenario.launch.py`, `scenario_utils.py`, `config/yolo_webviz_overlay.yaml` (multi-module integration).
