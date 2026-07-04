# Copyright 2026 TOPSBOT contributors.
# SPDX-License-Identifier: Apache-2.0
#
# Parameter merge order (later wins):
#   1. params_file — full scene yaml under config/
#   2. launch overrides — input_topic, input_msg, tb_img_profile, detection, channel, ws
#
# List built-in configs: ls $(ros2 pkg prefix topsbot_webviz)/share/topsbot_webviz/config/*.yaml

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


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
    web_dir = os.path.join(pkg_share, 'web')

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
            cwd=web_dir,
            output='screen',
        ),
    ]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'params_file',
            default_value='tbmem_jpeg_zc.yaml',
            description='Full params yaml under config/ (or absolute path)',
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
            description='Override TbMsg buffer tier for input_msg=tb_img: 480p | 540p | 1080p',
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
