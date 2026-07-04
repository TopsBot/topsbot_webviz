# Changelog

## 0.1.0

- Initial C++ webviz_node + browser frontend.

### Changed

- Config: remove `config/presets/` and `webviz_common.yaml`; use full flat `config/*.yaml` only.
- Launch: `preset` → `params_file`; align merge order with TOPSBOT launch/config spec.
- Subscription params: `input_topic`, `input_msg`, `tb_img_profile` (aligned with topsbot_cv / topsbot_yolo_det).
- Remove `image_type` / `image_topic`; legacy `input_mode` / `input_compressed` still accepted with WARN.
- Fix MatchQueue `strict` blocking image-only preview when `detection_enabled` is false.
- Docs: `QUICKSTART.md`, `config/README.md`, `config/params_reference.yaml`.

### Removed

- `config/presets/`, `config/webviz_common.yaml`, `config/4ch.yaml` alias.
- Launch argument `preset`; ROS params `image_type`, `image_topic`.
- `scenario.launch.py`, `scenario_utils.py`, `config/yolo_webviz_overlay.yaml` (multi-module integration).
