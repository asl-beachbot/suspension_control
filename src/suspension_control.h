#include "ros/ros.h"
#include "serial_com.h"
#include "sensor_msgs/Imu.h"

class SuspensionControl {
 public:

  SuspensionControl() {
    const std::string addr = "/dev/ttyUSB0";
    serial_com_.Open(addr);
    ROS_INFO("Opened suspension controller at %s", addr.c_str());

    const std::string topic = "imu/data";
    imu_sub_ = n_.subscribe(topic, 1, &SuspensionControl::ImuCallback, this);
    ROS_INFO("Suscribe to IMU topic \"%s\"", topic.c_str());
  }

  ~SuspensionControl() {
    delete &serial_com_;
  }
  
 private:
  ros::NodeHandle n_;
  ros::Subscriber imu_sub_;
  SerialCom serial_com_;
  sensor_msgs::Imu attitude_;
  double pitch_set_;
  double roll_set_;

  void ImuCallback(const sensor_msgs::Imu attitude) {
  	//TODO: rotate CS z = x axis; x = -z axis
  	attitude_ = attitude;
  }

};