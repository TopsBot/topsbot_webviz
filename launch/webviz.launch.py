# Copyright 2026 TOPSBOT contributors.
# SPDX-License-Identifier: Apache-2.0
#
# Single-channel webviz. Type yaml under config/ (tb_jpeg / tb_img / …).
# Default: tb_jpeg.yaml
#
#   ros2 launch topsbot_webviz webviz.launch.py
#   ros2 launch topsbot_webviz webviz.launch.py params_file:=tb_img.yaml
# Browser: http://<board-ip>:8000/

import os
import sys

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

_launch_dir = os.path.dirname(os.path.abspath(__file__))
if _launch_dir not in sys.path:
    sys.path.insert(0, _launch_dir)
from web_http_utils import load_ros_params, prepare_web_http_root  # noqa: E402


def _optional_bool(value: str):
    if not value:
        return None
    return value.lower() in ('true', '1', 'yes')


def _optional_int(value: str):
    if not value:
        return None
    return int(value)


def _resolve_params_path(pkg_share: str, params_file: str) -> str:
    if not params_file:
        raise RuntimeError('webviz.launch: params_file is empty')
    if os.path.isabs(params_file):
        path = params_file
    else:
        path = os.path.join(pkg_share, 'config', params_file)
    if not os.path.isfile(path):
        raise RuntimeError(f'webviz.launch: params_file not found: {path}')
    return path


def _launch_setup(context, *args, **kwargs):
    pkg_share = get_package_share_directory('topsbot_webviz')
    params_path = _resolve_params_path(
        pkg_share, LaunchConfiguration('params_file').perform(context))

    yaml_params = load_ros_params(params_path)
    max_channels = int(yaml_params.get('max_channel_count', yaml_params.get('channel_count', 1)))
    channel_count = int(yaml_params.get('channel_count', 1))
    if max_channels != 1 or channel_count != 1:
        raise RuntimeError(
            f'webviz.launch: params_file must be single-channel '
            f'(max_channel_count=1, channel_count=1), got max={max_channels} '
            f'count={channel_count}; use webviz_multi.launch.py')

    parameters = [params_path]
    overrides = {}

    for launch_key, ros_key in (
        ('input_topic', 'input_topic'),
        ('input_msg', 'input_msg'),
        ('tb_img_profile', 'tb_img_profile'),
        ('detection_topic', 'detection_topic'),
        ('stamp_align', 'stamp_align'),
        ('channel_name', 'channel_name'),
    ):
        value = LaunchConfiguration(launch_key).perform(context)
        if value:
            overrides[ros_key] = value

    detection_enabled = _optional_bool(LaunchConfiguration('detection_enabled').perform(context))
    if detection_enabled is not None:
        overrides['detection_enabled'] = detection_enabled

    enable_tacv_jpeg = _optional_bool(
        LaunchConfiguration('enable_tacv_jpeg').perform(context))
    if enable_tacv_jpeg is not None:
        overrides['enable_tacv_jpeg'] = enable_tacv_jpeg

    for launch_key, ros_key in (
        ('channel_id', 'channel_id'),
        ('jpeg_quality', 'jpeg_quality'),
        ('ws_base_port', 'ws_base_port'),
        ('queue_max', 'queue_max'),
    ):
        parsed = _optional_int(LaunchConfiguration(launch_key).perform(context))
        if parsed is not None:
            overrides[ros_key] = parsed

    if overrides:
        parameters.append(overrides)

    http_port = LaunchConfiguration('http_port').perform(context)
    merged = dict(yaml_params)
    merged.update(overrides)
    http_root = prepare_web_http_root(pkg_share, merged)

    return [
        Node(
            package='topsbot_webviz',
            executable='webviz_node',
            name='webviz_node',
            output='screen',
            parameters=parameters,
        ),
        ExecuteProcess(
            cmd=['python3', '-m', 'http.server', http_port],
            cwd=http_root,
            output='screen',
        ),
    ]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'params_file',
            default_value='tb_jpeg.yaml',
            description=(
                'Single-channel type yaml under config/ '
                '(tb_jpeg.yaml | tb_img.yaml | tb_img_yolo.yaml | '
                'sensor_image.yaml | sensor_compressed.yaml)'
            ),
        ),
        DeclareLaunchArgument(
            'input_topic',
            default_value='',
            description='Override input_topic',
        ),
        DeclareLaunchArgument(
            'input_msg',
            default_value='',
            description='Override input_msg: tb_jpeg | tb_img | sensor_image | sensor_compressed',
        ),
        DeclareLaunchArgument(
            'tb_img_profile',
            default_value='',
            description='Override TbMsg tier for input_msg=tb_img: 480p | 540p | 1080p',
        ),
        DeclareLaunchArgument(
            'detection_enabled',
            default_value='',
            description='Override detection_enabled (true/false)',
        ),
        DeclareLaunchArgument(
            'detection_topic',
            default_value='',
            description='Override detection_topic',
        ),
        DeclareLaunchArgument(
            'stamp_align',
            default_value='',
            description='Override stamp_align (strict/relaxed)',
        ),
        DeclareLaunchArgument(
            'channel_name',
            default_value='',
            description='Override channel_name',
        ),
        DeclareLaunchArgument(
            'channel_id',
            default_value='',
            description='Override channel_id',
        ),
        DeclareLaunchArgument(
            'jpeg_quality',
            default_value='',
            description='Override jpeg_quality',
        ),
        DeclareLaunchArgument(
            'enable_tacv_jpeg',
            default_value='',
            description='Override JPEG encoder: true = ta-cv first; false = OpenCV only',
        ),
        DeclareLaunchArgument(
            'ws_base_port',
            default_value='',
            description='Override ws_base_port',
        ),
        DeclareLaunchArgument(
            'queue_max',
            default_value='',
            description='Override queue_max',
        ),
        DeclareLaunchArgument(
            'http_port',
            default_value='8000',
            description='Static HTTP server port for browser UI',
        ),
        OpaqueFunction(function=_launch_setup),
    ])
