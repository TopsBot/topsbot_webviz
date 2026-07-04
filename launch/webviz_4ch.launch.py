# Copyright 2026 TOPSBOT contributors.
# SPDX-License-Identifier: Apache-2.0
#
# 四路 webviz：gateway 单节点，multi_instance 拆 4 节点
#
#   ros2 launch topsbot_webviz webviz_4ch.launch.py params_file:=4ch_gateway.yaml

import os

import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _load_ros_params(path: str) -> dict:
    with open(path, 'r', encoding='utf-8') as handle:
        data = yaml.safe_load(handle) or {}
    return data.get('/**', {}).get('ros__parameters', data)


def _single_channel_overrides(params: dict, index: int) -> dict:
    prefix = f'ch{index}_'
    input_msg = params.get(f'{prefix}input_msg', 'tb_jpeg')
    tb_profile = params.get(f'{prefix}tb_img_profile', '480p')
    return {
        'deployment': 'multi_instance',
        'stamp_align': params.get('stamp_align', 'strict'),
        'ws_base_port': params.get('ws_base_port', 8080),
        'queue_max': params.get('queue_max', 50),
        'channel_count': 1,
        'channel_id': params.get(f'{prefix}id', index),
        'channel_name': params.get(f'{prefix}name', f'Cam-{index + 1:02d}'),
        'input_topic': params.get(f'{prefix}input_topic', '/tbmem_jpeg'),
        'input_msg': input_msg,
        'tb_img_profile': tb_profile,
        'jpeg_quality': params.get('jpeg_quality', 75),
        'detection_topic': params.get(f'{prefix}detection_topic', '/detections'),
        'detection_enabled': params.get(f'{prefix}detection_enabled', False),
    }


def _resolve_params_path(pkg_share: str, params_file: str) -> str:
    if os.path.isabs(params_file):
        path = params_file
    else:
        path = os.path.join(pkg_share, 'config', params_file)
    if not os.path.isfile(path):
        raise RuntimeError(f'webviz_4ch.launch: params_file not found: {path}')
    return path


def _launch_setup(context, *args, **kwargs):
    pkg_share = get_package_share_directory('topsbot_webviz')
    params_file = LaunchConfiguration('params_file').perform(context)
    path = _resolve_params_path(pkg_share, params_file)
    params = _load_ros_params(path)

    http_port = LaunchConfiguration('http_port').perform(context)
    web_dir = os.path.join(pkg_share, 'web')
    deployment = params.get('deployment', 'multi_instance')
    actions = [
        ExecuteProcess(
            cmd=['python3', '-m', 'http.server', http_port],
            cwd=web_dir,
            output='screen',
        ),
    ]

    if deployment == 'gateway':
        actions.insert(0, Node(
            package='topsbot_webviz',
            executable='webviz_node',
            name='webviz_node',
            output='screen',
            parameters=[path],
        ))
        return actions

    channel_count = int(params.get('channel_count', 4))
    for index in range(channel_count):
        ch_id = int(params.get(f'ch{index}_id', index))
        actions.insert(0, Node(
            package='topsbot_webviz',
            executable='webviz_node',
            name=f'webviz_node_ch{ch_id}',
            output='screen',
            parameters=[_single_channel_overrides(params, index)],
        ))
    return actions


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'params_file',
            default_value='4ch_gateway.yaml',
            description='4-channel full params yaml under config/',
        ),
        DeclareLaunchArgument(
            'http_port',
            default_value='8000',
            description='Static HTTP server port',
        ),
        OpaqueFunction(function=_launch_setup),
    ])
