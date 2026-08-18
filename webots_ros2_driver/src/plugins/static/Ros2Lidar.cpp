// Copyright 1996-2023 Cyberbotics Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <webots_ros2_driver/plugins/static/Ros2Lidar.hpp>

#include <geometry_msgs/msg/transform_stamped.hpp>
#include <sensor_msgs/msg/point_field.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>

#include <webots/robot.h>

namespace webots_ros2_driver {
  void Ros2Lidar::init(webots_ros2_driver::WebotsNode *node, std::unordered_map<std::string, std::string> &parameters) {
    mLidar = wb_robot_get_device(parameters["name"].c_str());
    assert(mLidar != 0);
    if (parameters.count("updateRate") == 0)
      parameters.insert({"updateRate", std::to_string(wb_lidar_get_frequency(mLidar))});

    Ros2SensorPlugin::init(node, parameters);
    mIsSensorEnabled = false;
    mIsPointCloudEnabled = false;

    // Laser publisher
    if (wb_lidar_get_number_of_layers(mLidar) == 1) {
      mLaserPublisher = mNode->create_publisher<sensor_msgs::msg::LaserScan>(mTopicName, rclcpp::SensorDataQoS().reliable());
      const int resolution = wb_lidar_get_horizontal_resolution(mLidar);
      mLaserMessage.header.frame_id = mFrameName;
      mLaserMessage.angle_increment = -wb_lidar_get_fov(mLidar) / (resolution - 1);
      mLaserMessage.angle_min = wb_lidar_get_fov(mLidar) / 2.0;
      mLaserMessage.angle_max = -wb_lidar_get_fov(mLidar) / 2.0;
      mLaserMessage.time_increment = (double)wb_lidar_get_sampling_period(mLidar) / (1000.0 * resolution);
      mLaserMessage.scan_time = (double)wb_lidar_get_sampling_period(mLidar) / 1000.0;
      mLaserMessage.range_min = wb_lidar_get_min_range(mLidar);
      mLaserMessage.range_max = wb_lidar_get_max_range(mLidar);
      mLaserMessage.ranges.resize(resolution);
    }

    // Point cloud publisher
    mPointCloudPublisher =
      mNode->create_publisher<sensor_msgs::msg::PointCloud2>(mTopicName + "/point_cloud", rclcpp::SensorDataQoS().reliable());
    mPointCloudMessage.header.frame_id = mFrameName;
    mPointCloudMessage.height = 1;
    mPointCloudMessage.is_dense = false;
    mPointCloudMessage.is_bigendian = false;

    // RGB-LiDAR:
    // Explicitly expose XYZ + packed RGB in the ROS PointCloud2.
    // This avoids coupling the ROS binary layout to sizeof(WbLidarPoint).
    sensor_msgs::PointCloud2Modifier modifier(mPointCloudMessage);
    modifier.setPointCloud2FieldsByString(2, "xyz", "rgb");

    if (mAlwaysOn) {
      wb_lidar_enable(mLidar, mPublishTimestepSyncedMs);
      wb_lidar_enable_point_cloud(mLidar);
      mIsSensorEnabled = true;
      mIsPointCloudEnabled = true;
    }
  }

  void Ros2Lidar::step() {
    if (!preStep())
      return;

    if (mIsSensorEnabled && mLaserPublisher != nullptr)
      publishLaserScan();

    if (mIsPointCloudEnabled)
      publishPointCloud();

    if (mAlwaysOn)
      return;

    const bool shouldPointCloudBeEnabled = mPointCloudPublisher->get_subscription_count() > 0;
    const bool shouldSensorBeEnabled =
      shouldPointCloudBeEnabled || (mLaserPublisher != nullptr && mLaserPublisher->get_subscription_count() > 0);

    // Enable/Disable sensor
    if (shouldSensorBeEnabled != mIsSensorEnabled) {
      if (shouldSensorBeEnabled)
        wb_lidar_enable(mLidar, mPublishTimestepSyncedMs);
      else
        wb_lidar_disable(mLidar);
      mIsSensorEnabled = shouldSensorBeEnabled;
    }

    // Enable/Disable point cloud
    if (shouldPointCloudBeEnabled != mIsPointCloudEnabled) {
      if (shouldPointCloudBeEnabled)
        wb_lidar_enable_point_cloud(mLidar);
      else
        wb_lidar_disable_point_cloud(mLidar);
      mIsPointCloudEnabled = shouldPointCloudBeEnabled;
    }
  }

  void Ros2Lidar::publishPointCloud() {
    const WbLidarPoint *data =
      wb_lidar_get_point_cloud(mLidar);

    if (!data)
      return;

    const int numberOfPoints =
      wb_lidar_get_number_of_points(mLidar);

    mPointCloudMessage.header.stamp =
      mNode->get_clock()->now();

    sensor_msgs::PointCloud2Modifier modifier(
      mPointCloudMessage);

    modifier.resize(numberOfPoints);

    sensor_msgs::PointCloud2Iterator<float> iterX(
      mPointCloudMessage,
      "x");

    sensor_msgs::PointCloud2Iterator<float> iterY(
      mPointCloudMessage,
      "y");

    sensor_msgs::PointCloud2Iterator<float> iterZ(
      mPointCloudMessage,
      "z");

    sensor_msgs::PointCloud2Iterator<uint8_t> iterR(
      mPointCloudMessage,
      "r");

    sensor_msgs::PointCloud2Iterator<uint8_t> iterG(
      mPointCloudMessage,
      "g");

    sensor_msgs::PointCloud2Iterator<uint8_t> iterB(
      mPointCloudMessage,
      "b");

    for (int i = 0;
        i < numberOfPoints;
        ++i,
        ++iterX,
        ++iterY,
        ++iterZ,
        ++iterR,
        ++iterG,
        ++iterB) {

      *iterX = data[i].x;
      *iterY = data[i].y;
      *iterZ = data[i].z;

      *iterR = data[i].r;
      *iterG = data[i].g;
      *iterB = data[i].b;
    }

    mPointCloudPublisher->publish(
      mPointCloudMessage);
  }

  void Ros2Lidar::publishLaserScan() {
    auto rangeImage = wb_lidar_get_layer_range_image(mLidar, 0);
    if (rangeImage) {
      memcpy(mLaserMessage.ranges.data(), rangeImage, mLaserMessage.ranges.size() * sizeof(float));
      mLaserMessage.header.stamp = mNode->get_clock()->now();
      mLaserPublisher->publish(mLaserMessage);
    }
  }

}  // end namespace webots_ros2_driver
