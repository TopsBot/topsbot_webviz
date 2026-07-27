# Copyright 2026 TOPSBOT contributors.
# SPDX-License-Identifier: Apache-2.0
#
# Multi-channel webviz.
#   params_file: type yaml (e.g. tb_jpeg_multi.yaml) with max_channel_count + ch0_…
#   deployment: multi_instance (default) | gateway
#   channel_count: optional launch override for how many to enable (1 … max)
#
#   ros2 launch topsbot_webviz webviz_multi.launch.py
#   ros2 launch topsbot_webviz webviz_multi.launch.py channel_count:=4
# Browser: http://<board-ip>:8000/

import os
import sys

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, LogInfo, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

_launch_dir = os.path.dirname(os.path.abspath(__file__))
if _launch_dir not in sys.path:
    sys.path.insert(0, _launch_dir)
from web_http_utils import load_ros_params, prepare_web_http_root  # noqa: E402


def _optional_int(value: str):
    if not value:
        return None
    return int(value)


def _optional_bool(value: str):
    if not value:
        return None
    return value.lower() in ('true', '1', 'yes')


def _single_channel_overrides(params: dict, index: int) -> dict:
    prefix = f'ch{index}_'
    input_msg = params.get(f'{prefix}input_msg', 'tb_jpeg')
    tb_profile = params.get(f'{prefix}tb_img_profile', '480p')
    return {
        'deployment': 'multi_instance',
        'stamp_align': params.get('stamp_align', 'strict'),
        'ws_base_port': params.get('ws_base_port', 8080),
        'queue_max': params.get('queue_max', 50),
        'max_channel_count': 1,
        'channel_count': 1,
        'channel_id': params.get(f'{prefix}id', index),
        'channel_name': params.get(f'{prefix}name', f'Cam-{index + 1:02d}'),
        'input_topic': params.get(f'{prefix}input_topic', '/tbmem_jpeg'),
        'input_msg': input_msg,
        'tb_img_profile': tb_profile,
        'jpeg_quality': params.get('jpeg_quality', 75),
        'enable_tacv_jpeg': params.get('enable_tacv_jpeg', True),
        'detection_topic': params.get(f'{prefix}detection_topic', '/detections'),
        'detection_enabled': params.get(f'{prefix}detection_enabled', False),
    }


def _resolve_params_path(pkg_share: str, params_file: str) -> str:
    if not params_file:
        raise RuntimeError('webviz_multi.launch: params_file is empty')
    if os.path.isabs(params_file):
        path = params_file
    else:
        path = os.path.join(pkg_share, 'config', params_file)
    if not os.path.isfile(path):
        raise RuntimeError(f'webviz_multi.launch: params_file not found: {path}')
    return path


def _max_and_selected(params: dict, requested):
    """yaml max_channel_count (fallback channel_count) = max; channel_count = default selected."""
    max_channels = int(params.get('max_channel_count', params.get('channel_count', 0)))
    if max_channels < 1:
        raise RuntimeError(
            'webviz_multi.launch: params_file max_channel_count/channel_count must be >= 1')

    default_selected = int(params.get('channel_count', max_channels))
    if default_selected < 1:
        default_selected = max_channels
    if default_selected > max_channels:
        default_selected = max_channels

    if requested is None:
        return max_channels, default_selected

    if requested < 1:
        raise RuntimeError(
            f'webviz_multi.launch: channel_count must be >= 1 (got {requested})')
    if requested > max_channels:
        raise RuntimeError(
            f'webviz_multi.launch: channel_count={requested} exceeds '
            f'max_channel_count={max_channels} in params_file')
    return max_channels, requested


def _launch_setup(context, *args, **kwargs):
    pkg_share = get_package_share_directory('topsbot_webviz')
    params_file = LaunchConfiguration('params_file').perform(context)
    path = _resolve_params_path(pkg_share, params_file)
    params = load_ros_params(path)

    requested = _optional_int(LaunchConfiguration('channel_count').perform(context))
    max_channels, channel_count = _max_and_selected(params, requested)

    http_port = LaunchConfiguration('http_port').perform(context)
    runtime_params = dict(params)
    runtime_params['max_channel_count'] = max_channels
    runtime_params['channel_count'] = channel_count
    enable_tacv_jpeg = _optional_bool(
        LaunchConfiguration('enable_tacv_jpeg').perform(context))
    if enable_tacv_jpeg is not None:
        runtime_params['enable_tacv_jpeg'] = enable_tacv_jpeg
    deployment_override = LaunchConfiguration('deployment').perform(context).strip()
    deployment = deployment_override or str(
        params.get('deployment', 'multi_instance')).strip() or 'multi_instance'
    runtime_params['deployment'] = deployment
    http_root = prepare_web_http_root(pkg_share, runtime_params)

    actions = [
        LogInfo(msg=(
            f'webviz_multi: deployment={deployment} '
            f'channels={channel_count}/{max_channels} (selected/max)'
        )),
        ExecuteProcess(
            cmd=['python3', '-m', 'http.server', http_port],
            cwd=http_root,
            output='screen',
        ),
    ]

    if deployment == 'gateway':
        launch_overrides = {
            'deployment': 'gateway',
            'max_channel_count': max_channels,
            'channel_count': channel_count,
        }
        if enable_tacv_jpeg is not None:
            launch_overrides['enable_tacv_jpeg'] = enable_tacv_jpeg
        node_params = [path, launch_overrides]
        actions.insert(0, Node(
            package='topsbot_webviz',
            executable='webviz_node',
            name='webviz_node',
            output='screen',
            parameters=node_params,
        ))
        return actions

    if deployment not in ('multi_instance', 'multi'):
        raise RuntimeError(
            f'webviz_multi.launch: unsupported deployment={deployment!r} '
            '(use gateway or multi_instance)')

    for index in range(channel_count):
        ch_id = int(params.get(f'ch{index}_id', index))
        actions.insert(0, Node(
            package='topsbot_webviz',
            executable='webviz_node',
            name=f'webviz_node_ch{ch_id}',
            output='screen',
            parameters=[_single_channel_overrides(runtime_params, index)],
        ))
    return actions


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'params_file',
            default_value='tb_jpeg_multi.yaml',
            description=(
                'Type yaml under config/ (default: tb_jpeg_multi.yaml). '
                'File sets max_channel_count + default channel_count'
            ),
        ),
        DeclareLaunchArgument(
            'channel_count',
            default_value='',
            description=(
                'Selected channels to enable (1 … max_channel_count). '
                'Empty = use channel_count from params_file'
            ),
        ),
        DeclareLaunchArgument(
            'deployment',
            default_value='',
            description=(
                'Optional deployment override: multi_instance | gateway. '
                'Empty uses deployment from params_file.'
            ),
        ),
        DeclareLaunchArgument(
            'enable_tacv_jpeg',
            default_value='',
            description=(
                'Optional JPEG encoder override: true = ta-cv first; '
                'false = OpenCV only. Empty uses params_file.'
            ),
        ),
        DeclareLaunchArgument(
            'http_port',
            default_value='8000',
            description='Static HTTP server port',
        ),
        OpaqueFunction(function=_launch_setup),
    ])
