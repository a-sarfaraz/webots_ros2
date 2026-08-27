# Ouster OS0 Rev 8 RGBD LiDAR — Webots + ROS 2

This project provides a Webots simulation model of an **Ouster OS0 Rev 8 RGBD LiDAR** with ROS 2 support. A ROSbot XL + Ouster OS0 Rev 8 reference integration is included.

The reference ROS integration exposes:

```text
Topic: /lidar/point_cloud
Type:  sensor_msgs/msg/PointCloud2
Frame: lidar
Fields: x, y, z, rgb
```

The cloud can be consumed directly by RViz, PCL, perception, mapping, and navigation pipelines.

---

# Getting Started

The complete RGBD LiDAR simulation uses two repositories:

```text
Modified Webots simulator:
https://github.com/a-sarfaraz/webots
branch: rgbd-lidar-r2025a

Modified webots_ros2 integration:
https://github.com/a-sarfaraz/webots_ros2
branch: rgbd-lidar-r2025a
```

Both repositories are required for the complete Webots + ROS 2 integration. The instructions below assume **Ubuntu 22.04** with **ROS 2 Humble** already installed.

## 1. Clone the Modified Webots Source

From your home directory, clone the Webots fork directly on the RGBD LiDAR branch:

```bash
git clone \
  --branch rgbd-lidar-r2025a \
  --single-branch \
  --recurse-submodules \
  https://github.com/a-sarfaraz/webots.git \
  webots-rgb-lidar
```

If the repository was cloned without `--recurse-submodules`, initialize the Webots submodules manually:

```bash
cd ~/webots-rgb-lidar
git submodule update --init --recursive
```

The RGBD LiDAR implementation is based on **Webots R2025a** and must currently be built from this source tree.

Build Webots from the repository root:

```bash
cd ~/webots-rgb-lidar
make -j$(nproc)
```

After a successful build, start the modified Webots executable with:

```bash
cd ~/webots-rgb-lidar
./webots
```

---

## 2. Set Up `webots_ros2`

Create the workspace, clone the modified ROS 2 integration, install dependencies, build it, and configure the environment:

```bash
mkdir -p ~/webots_ros2_ws/src
cd ~/webots_ros2_ws/src

git clone \
  --branch rgbd-lidar-r2025a \
  --single-branch \
  https://github.com/a-sarfaraz/webots_ros2.git

cd ~/webots_ros2_ws

source /opt/ros/humble/setup.bash

rosdep install \
  --from-paths src \
  --ignore-src \
  -r \
  -y

colcon build --symlink-install
```

Add the required environment setup to `~/.bashrc` so every new terminal is ready automatically:

```bash
cat >> ~/.bashrc <<'EOF'

source /opt/ros/humble/setup.bash
source ~/webots_ros2_ws/install/setup.bash
export RGBD_WEBOTS_HOME=~/webots-rgb-lidar
EOF

source ~/.bashrc
```

Launch the ROSbot XL reference example with:

```bash
ros2 launch webots_ros2_husarion rosbot_xl_os0_launch.py
```

---

# Adding a Robot to the World

For adding and configuring robots in Webots, refer to the official Webots documentation:

- [Webots Tutorials](https://cyberbotics.com/doc/guide/tutorials)
- [Scene Tree and World Structure](https://cyberbotics.com/doc/guide/the-scene-tree)
- [Creating and Using PROTO Nodes](https://cyberbotics.com/doc/guide/tutorial-7-your-first-proto)

---

# Adding an RGBD LiDAR

This project provides:

- a generic `RgbdLidar`
- an `OusterOS0Rev8` wrapper built on top of it

## Generic RGBD LiDAR

The generic RGBD LiDAR model is defined in the modified Webots repository:

[projects/robots/generic/rgbd_lidar/protos/RgbdLidar.proto](https://github.com/a-sarfaraz/webots/blob/rgbd-lidar-r2025a/projects/robots/generic/rgbd_lidar/protos/RgbdLidar.proto)

Import the PROTO near the top of your `.wbt` world file:

```webots
EXTERNPROTO "../../../../robots/generic/rgbd_lidar/protos/RgbdLidar.proto"
```

The path is relative to the `.wbt` file; adjust it if your world is stored in a different directory.

Then attach the sensor to the robot using an appropriate sensor field. For example:

```webots
lidarSlot [
  RgbdLidar {
    name "lidar"
    horizontalResolution 1024
    numberOfLayers 32
  }
]
```

Key parameters include:

```text
horizontalResolution
fieldOfView
verticalFieldOfView
numberOfLayers
minRange
maxRange
```

## Ouster OS0 Rev 8

The Ouster OS0 Rev 8 wrapper is defined in the modified Webots repository:

[projects/robots/ouster/rev8/protos/OusterOS0Rev8.proto](https://github.com/a-sarfaraz/webots/blob/rgbd-lidar-r2025a/projects/robots/ouster/rev8/protos/OusterOS0Rev8.proto)

Import the PROTO near the top of your `.wbt` world file:

```webots
EXTERNPROTO "../../../../robots/ouster/rev8/protos/OusterOS0Rev8.proto"
```

Again, the path is relative to the location of your `.wbt` file.

Attach the sensor to the robot:

```webots
lidarSlot [
  OusterOS0Rev8 {
    translation 0 0 0.0225
    name "os0"
    lidarName "lidar"
    horizontalResolution 2048
    numberOfLayers 128
  }
]
```

The OS0 wrapper uses the generic RGBD LiDAR implementation with OS0-specific geometry and sensor parameters.

It defaults to `1024 × 128`; the validated high-resolution configuration uses `2048 × 128`.

For robots without a `lidarSlot`, use the appropriate attachment field such as `children`, `sensorSlot`, or another robot-specific slot.

---

# Connecting the Webots Robot to ROS 2

To allow `webots_ros2_driver` to control the robot, configure it in the Webots world with:

```webots
Robot {
  name "my_robot"
  controller "<extern>"
}
```

`controller "<extern>"` tells Webots that the robot will be controlled by an external process rather than by a controller running inside Webots.

On the ROS 2 side, `WebotsController` is the `webots_ros2_driver` process that connects to a specific robot in the running Webots simulation and exposes its devices and control interface to ROS 2.

The `WebotsController` must target the same robot name used in the Webots world. For example:

```webots
name "my_robot"
```

must correspond to a `WebotsController` configured for `my_robot`.

For the complete workflow, refer to the official ROS 2 Humble Webots tutorials:

- [Webots with ROS 2](https://docs.ros.org/en/humble/Tutorials/Advanced/Simulators/Webots.html)
- [Setting up a Robot Simulation with Webots](https://docs.ros.org/en/humble/Tutorials/Advanced/Simulators/Webots/Setting-Up-Simulation-Webots-Basic.html)
- [Advanced Webots ROS 2 Integration](https://docs.ros.org/en/humble/Tutorials/Advanced/Simulators/Webots/Setting-Up-Simulation-Webots-Advanced.html)

---

# ROS Interface

The reference integration exposes the LiDAR point cloud on:

```text
/lidar/point_cloud
```

with message type:

```text
sensor_msgs/msg/PointCloud2
```

Each point contains:

```text
x    FLOAT32   Cartesian X coordinate [m]
y    FLOAT32   Cartesian Y coordinate [m]
z    FLOAT32   Cartesian Z coordinate [m]
rgb  FLOAT32   Packed RGB value
```

The `rgb` field follows the standard ROS/PCL packed RGB representation. The red, green, and blue channels are packed into the bits of a 32-bit value and stored in the `PointCloud2` field as `FLOAT32`.

This allows the cloud to be consumed directly by common ROS perception tools:

```text
RViz        -> PointCloud2 display with Color Transformer = RGB8
PCL         -> pcl::PointXYZRGB
ROS nodes   -> unpack rgb into individual R/G/B channels
Mapping/CV  -> use XYZ for geometry and RGB for visual or semantic processing
```

Conceptually, each point represents:

```text
(x, y, z) + (r, g, b)
```

where RGB corresponds to the same simulated LiDAR sample as the XYZ measurement.

For custom integrations, the underlying device topic may differ and can be remapped to the desired public topic.

---

# RViz

For the provided ROSbot XL reference example:

```text
Fixed Frame: map
Display: PointCloud2
Topic: /lidar/point_cloud
Position Transformer: XYZ
Color Transformer: RGB8
```

For custom integrations, use any fixed frame with a valid TF path to `lidar`.

---

# Important Files

## Webots Fork

Ouster OS0 model:

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

## `webots_ros2` Fork

ROS point-cloud publisher:

```text
webots_ros2_driver/src/plugins/static/Ros2Lidar.cpp
```

ROSbot XL reference launch:

```text
webots_ros2_husarion/launch/rosbot_xl_os0_launch.py
```

---