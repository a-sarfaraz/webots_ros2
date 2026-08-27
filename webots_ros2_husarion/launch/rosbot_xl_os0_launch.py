#!/usr/bin/env python3

import os

from ament_index_python.packages import (
    get_package_prefix,
    get_package_share_directory,
)

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    EmitEvent,
    ExecuteProcess,
    RegisterEventHandler,
    TimerAction,
)
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.substitutions import (
    EnvironmentVariable,
    LaunchConfiguration,
    PathJoinSubstitution,
)

from launch_ros.actions import Node
from launch.conditions import IfCondition

def generate_launch_description():
    husarion_share = get_package_share_directory(
        'webots_ros2_husarion'
    )

    driver_share = get_package_share_directory(
        'webots_ros2_driver'
    )

    driver_prefix = get_package_prefix(
        'webots_ros2_driver'
    )

    webots_home = LaunchConfiguration('webots_home')
    world = LaunchConfiguration('world')
    port = LaunchConfiguration('port')
    rviz = LaunchConfiguration('rviz')

    webots_executable = PathJoinSubstitution([
        webots_home,
        'webots',
    ])

    world_path = PathJoinSubstitution([
        webots_home,
        'projects',
        'samples',
        'environments',
        'indoor',
        'worlds',
        world,
    ])

    robot_description = os.path.join(
        husarion_share,
        'resource',
        'rosbot_webots.urdf',
    )

    controller_config = os.path.join(
        husarion_share,
        'resource',
        'rosbot_xl_controllers.yaml',
    )

    rviz_config = os.path.join(
        husarion_share,
        'resource',
        'rosbot_xl_os0.rviz',
    )

    webots_controller = os.path.join(
        driver_share,
        'scripts',
        'webots-controller',
    )

    # Start the modified Webots build directly.
    #
    # We deliberately do not use WebotsLauncher yet because our
    # break-room world contains the normal Webots Supervisor controller
    # "ground_truth_logger". Keeping the original world path preserves
    # normal Webots controller discovery during this first integration.
    webots = ExecuteProcess(
        cmd=[
            webots_executable,
            ['--port=', port],
            world_path,
        ],
        output='screen',
        name='webots',
    )

    # Equivalent to the manually validated Webots ROS driver command.
    rosbot_driver = ExecuteProcess(
        cmd=[
            webots_controller,
            '--robot-name=rosbot_xl',
            '--protocol=ipc',
            ['--port=', port],
            'ros2',
            '--ros-args',
            '-p',
            'robot_description:=' + robot_description,
            '--params-file',
            controller_config,

            '-r',
            'rosbot_xl_base_controller/cmd_vel_unstamped:=/cmd_vel',

            '-r',
            'rosbot_xl_base_controller/cmd_vel:=/cmd_vel',

            # Public RGBD LiDAR interface.
            '-r',
            '/rosbot_xl/lidar/point_cloud:=/lidar/point_cloud',
        ],
        output='screen',
        name='rosbot_xl_webots_driver',

        # Matches the environment used by the Webots ROS controller
        # infrastructure.
        additional_env={
            'WEBOTS_HOME': driver_prefix,
        },
    )

    joint_state_broadcaster = Node(
        package='controller_manager',
        executable='spawner',
        arguments=[
            'joint_state_broadcaster',
            '--controller-manager-timeout',
            '50',
        ],
        output='screen',
    )

    rosbot_base_controller = Node(
        package='controller_manager',
        executable='spawner',
        arguments=[
            'rosbot_xl_base_controller',
            '--controller-manager-timeout',
            '50',
        ],
        output='screen',
    )
    map_to_odom_tf = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='map_to_odom_tf',
        arguments=[
            '--x', '0',
            '--y', '0',
            '--z', '0',
            '--roll', '0',
            '--pitch', '0',
            '--yaw', '0',
            '--frame-id', 'map',
            '--child-frame-id', 'odom',
        ],
        output='screen',
    )

    lidar_static_tf = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='lidar_static_tf',
        arguments=[
            '--x', '0',
            '--y', '0',
            '--z', '0.194192',
            '--roll', '0',
            '--pitch', '0',
            '--yaw', '0',
            '--frame-id', 'base_link',
            '--child-frame-id', 'lidar',
        ],
        output='screen',
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=[
            '-d',
            rviz_config,
        ],
        output='screen',
        condition=IfCondition(rviz),
    )

    # Give Webots a short head start before connecting the external
    # ROS controller. Controller spawners themselves can wait up to
    # 50 seconds for controller_manager.
    ros_nodes = TimerAction(
        period=2.0,
        actions=[
            rosbot_driver,
            joint_state_broadcaster,
            rosbot_base_controller,
	    map_to_odom_tf,
            lidar_static_tf,
	    rviz_node,
        ],
    )

    shutdown_when_webots_exits = RegisterEventHandler(
        OnProcessExit(
            target_action=webots,
            on_exit=[
                EmitEvent(event=Shutdown()),
            ],
        )
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'webots_home',
            default_value=EnvironmentVariable(
                'RGBD_WEBOTS_HOME',
                default_value=os.path.expanduser(
                    '~/webots-rgb-lidar'
                ),
            ),
            description=(
                'Path to the modified Webots source/build containing '
                'the RGBD LiDAR implementation.'
            ),
        ),

        DeclareLaunchArgument(
            'world',
            default_value='rosbot_xl_os0_break_room.wbt',
            description='Webots world filename.',
        ),

        DeclareLaunchArgument(
            'port',
            default_value='1234',
            description='Webots external-controller port.',
        ),

	DeclareLaunchArgument(
            'rviz',
            default_value='true',
            description='Start RViz with the RGBD LiDAR configuration.',
        ),

        webots,
        ros_nodes,
        shutdown_when_webots_exits,
    ])
