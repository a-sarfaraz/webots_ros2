# Ouster OS0 Rev 8 RGBD LiDAR — Webots + ROS 2

This project provides a Webots simulation model of an **Ouster OS0 Rev 8 RGBD LiDAR** with ROS 2 support.

The provided ROS integration exposes the LiDAR point cloud on:

```text
Topic: /lidar/point_cloud
Type:  sensor_msgs/msg/PointCloud2
Frame: lidar
```

The point cloud contains:

```text
x
y
z
rgb
```

and can be consumed directly by RViz, PCL, perception, mapping, and navigation pipelines.

---

# Getting Started

Clone this fork directly on the RGBD LiDAR branch:

```bash
git clone \
  --branch rgbd-lidar-r2025a \
  --single-branch \
  --recurse-submodules \
  https://github.com/a-sarfaraz/webots.git \
  webots-rgb-lidar
```

Enter the repository:

```bash
cd webots-rgb-lidar
```

If the repository was cloned without `--recurse-submodules`, initialize the Webots submodules manually:

```bash
git submodule update --init --recursive
```

The RGBD LiDAR implementation is based on **Webots R2025a** and must currently be built from this source tree.

Build Webots with:

```bash
make -j$(nproc)
```

After a successful build, start Webots with:

```bash
./webots
```

or open a specific world directly:

```bash
./webots path/to/world.wbt
```

For example:

```bash
./webots \
  projects/samples/environments/indoor/worlds/break_room.wbt
```

At this point you have the modified Webots installation containing:

```text
Generic RGBD LiDAR
Ouster OS0 Rev 8 model
XYZ + RGB LiDAR point support
RGB/range correspondence implementation
```

The following sections explain how to:

1. choose or create a Webots environment,
2. add your robot,
3. mount the Ouster OS0 Rev 8,
4. connect the robot to ROS 2,
5. publish `/lidar/point_cloud`,
6. configure TF and RViz.

---

# Setting Up a Webots Simulation From Scratch

A Webots simulation normally consists of:

```text
World (.wbt)
 ├── environment
 ├── robot
 └── sensors attached to the robot
```

The `.wbt` world describes the complete simulation scene.

You can start from one of the existing Webots sample environments rather than building a world from scratch.

For example:

```text
projects/samples/environments/indoor/worlds/
projects/samples/environments/factory/worlds/
```

## Opening a World

From the Webots source directory:

```bash
cd ~/webots-rgb-lidar
```

Launch a world with:

```bash
./webots path/to/world.wbt
```

For example:

```bash
./webots \
  projects/samples/environments/indoor/worlds/break_room.wbt
```

You can then inspect the environment in the Webots GUI before adding the robot.

---

# Adding a Robot to the World

Robots are usually defined using Webots PROTO files.

For example, the ROSbot XL PROTO is:

```text
projects/robots/husarion/rosbot_xl/protos/RosbotXl.proto
```

Import the robot PROTO near the top of the world:

```webots
EXTERNPROTO "../../../../robots/husarion/rosbot_xl/protos/RosbotXl.proto"
```

The robot can be added as a
**top-level node in the world**, alongside the other environment objects.

```webots
RosbotXl {
  translation 0 0 0
  rotation 0 0 1 0
  name "rosbot_xl"
}
```
For ROS integration, the robot should normally use:

```webots
controller "<extern>"
```

For example:

```webots
RosbotXl {
  translation 0 0 0
  rotation 0 0 1 0
  name "rosbot_xl"
  controller "<extern>"
}
```

`<extern>` means that the robot controller will be provided externally by `webots_ros2_driver` rather than by a controller running directly inside Webots.

---

# Adding the Ouster OS0 Rev 8

The Ouster model is defined at:

```text
projects/robots/ouster/rev8/protos/OusterOS0Rev8.proto
```

Import it into your world or robot PROTO:

```webots
EXTERNPROTO "../../../../robots/ouster/rev8/protos/OusterOS0Rev8.proto"
```

If your robot provides a LiDAR or sensor slot, place the sensor inside that slot.

Example:

```webots
RosbotXl {
  translation 0 0 0
  rotation 0 0 1 0
  name "rosbot_xl"
  controller "<extern>"

  lidarSlot [
    OusterOS0Rev8 {
      translation 0 0 0.0225
      name "os0"
      lidarName "lidar"
      horizontalResolution 2048
      numberOfLayers 128
    }
  ]
}
```

For another robot, the exact field may not be called `lidarSlot`.

Depending on the robot PROTO, the sensor may instead need to be added to:

```text
children
sensorSlot
lidarSlot
topSlot
```

or another robot-specific extension field.

The important requirement is that the `OusterOS0Rev8` becomes part of the robot's rigid body hierarchy.

---

# Sensor Mounting

Position the `OusterOS0Rev8` node at the desired mounting point on the robot.

The PROTO already accounts for the OS0's internal LiDAR beam-origin offset of:

```text
0 0 0.040242 m
```

so this offset normally does not need to be added manually.

---

# Choosing the LiDAR Resolution

The OS0 wrapper defaults to:

```text
Horizontal resolution: 1024
Vertical layers:       128
```

The high-resolution configuration used during testing is:

```webots
OusterOS0Rev8 {
  horizontalResolution 2048
  numberOfLayers 128
}
```

---

# Setting Up `webots_ros2`

The Webots simulator and the ROS 2 interface are maintained as separate source repositories.

After building the modified Webots source tree, create a ROS 2 workspace:

```bash
mkdir -p ~/webots_ros2_ws/src
cd ~/webots_ros2_ws/src
```

Clone the `webots_ros2` repository containing the RGBD LiDAR ROS integration:

```bash
git clone <WEBOTS_ROS2_REPOSITORY_URL>
cd webots_ros2
git checkout rgbd-lidar-r2025a
```

Replace `<WEBOTS_ROS2_REPOSITORY_URL>` with the corresponding `webots_ros2` fork containing:

```text
RGBD PointCloud2 support in Ros2Lidar.cpp
the Ouster reference launch file
the ROSbot XL integration example
```

Build the ROS 2 workspace:

```bash
cd ~/webots_ros2_ws

source /opt/ros/humble/setup.bash

colcon build --symlink-install
```

Source the workspace:

```bash
source /opt/ros/humble/setup.bash
source ~/webots_ros2_ws/install/setup.bash
```

Point the ROS integration to the modified Webots build:

```bash
export RGBD_WEBOTS_HOME=~/webots-rgb-lidar
```

The ROSbot XL reference integration can then be launched with:

```bash
ros2 launch \
  webots_ros2_husarion \
  rosbot_xl_os0_launch.py
```

---

# Connecting the Webots Robot to ROS 2

Once the world contains:

```text
environment
+
robot
+
OusterOS0Rev8
```

the next step is to connect the robot to `webots_ros2`.

The robot should have:

```webots
controller "<extern>"
```

The ROS side then launches a `WebotsController` for that robot.

The robot name used by the ROS controller must match the Webots robot name.

For example:

```webots
name "rosbot_xl"
```

should correspond to:

```text
--robot-name=rosbot_xl
```

in the ROS launch configuration.

The public LiDAR topic exposed by the provided ROS integration is:

```text
/lidar/point_cloud
```

If the underlying driver initially publishes a robot-specific topic, remap it in the launch file to `/lidar/point_cloud`.

---

# ROS Interface

The public LiDAR topic exposed by the provided ROS integration is:

```text
/lidar/point_cloud
```

with message type:

```text
sensor_msgs/msg/PointCloud2
```

The cloud contains:

```text
x
y
z
rgb
```

The RGB field uses the standard packed ROS/PCL representation.
---

# TF and RViz

A typical TF tree is:

```text
map
 └── odom
      └── base_link
           └── lidar
```

The reference simulation currently uses Webots ground truth for:

```text
odom -> base_link
```

The LiDAR mounting transform remains:

```text
base_link -> lidar
```

## RViz

Recommended configuration:

```text
Fixed Frame: map

Display:
PointCloud2

Topic:
/lidar/point_cloud

Position Transformer:
XYZ

Color Transformer:
RGB8
```

If the cloud does not appear, first check:

```bash
ros2 topic info /lidar/point_cloud
```

Then check the TF chain:

```bash
ros2 run tf2_ros tf2_echo map lidar
```

Both the topic and a valid TF path must exist for RViz to display the cloud correctly.

---

# Reference ROSbot XL Example

A working reference integration is provided using:

```text
ROSbot XL
+
Ouster OS0 Rev 8
+
Webots environment
+
webots_ros2
```

The launch entry point is:

```bash
ros2 launch \
  webots_ros2_husarion \
  rosbot_xl_os0_launch.py
```
This example should be treated as a template for integrating the sensor with another robot.

The robot-specific parts are mainly:

```text
robot PROTO
sensor mounting position
robot_description
ros2_control configuration
base_link -> lidar TF
```

The LiDAR model and ROS point-cloud interface remain the same.

---

# Verifying the Point Cloud

Check that the topic exists:

```bash
ros2 topic info /lidar/point_cloud
```

Expected message type:

```text
sensor_msgs/msg/PointCloud2
```
Inspect the PointCloud2 fields:

```bash
ros2 topic echo \
  /lidar/point_cloud \
  --once \
  --field fields
```

The cloud should contain at least:

```text
x
y
z
rgb
```
---
# Important Files

Webots OS0 model:

```text
projects/robots/ouster/rev8/protos/OusterOS0Rev8.proto
```

Generic RGBD LiDAR:

```text
projects/robots/generic/rgbd_lidar/protos/RgbdLidar.proto
```

OS0 visual model:

```text
projects/robots/ouster/rev8/meshes/os0/os0_rev8_visual.obj
```

Webots RGBD LiDAR implementation:

```text
src/webots/nodes/WbLidar.cpp
src/webots/wren/WbWrenCamera.cpp
```

RGB/range shaders:

```text
resources/wren/shaders/pack_rgb_range.frag
resources/wren/shaders/merge_spherical_rgb_packed.frag
```

ROS point-cloud publisher:

```text
webots_ros2_driver/src/plugins/static/Ros2Lidar.cpp
```

ROSbot XL reference launch:

```text
webots_ros2_husarion/launch/rosbot_xl_os0_launch.py
```

---
