#include "ros/ros.h"
#include "serial_com.h"
#include "sensor_msgs/Imu.h"
#include "geometry_msgs/PoseStamped.h"
#include <Eigen/Geometry>
#include <cmath>

class SuspensionControl {
 public:

  SuspensionControl() {
    const std::string addr = "/dev/ttyUSB0";
    //serial_com_.Open(addr);
    ROS_INFO("Opened suspension controller at %s", addr.c_str());

    const std::string topic = "/imu/data";
    imu_sub_ = n_.subscribe(topic, 1, &SuspensionControl::ImuCallback, this);
    ROS_INFO("Suscribe to IMU topic \"%s\"", topic.c_str());

    const std::string topic_pub = "imu_test";
    imu_pub_ = n_.advertise<geometry_msgs::PoseStamped>(topic_pub, 1);
    ROS_INFO("Publishing on topic \"%s\"", topic_pub.c_str());
  }

  ~SuspensionControl() {
    //delete &serial_com_;
  }
  
 private:
  ros::NodeHandle n_;
  ros::Subscriber imu_sub_;
  ros::Publisher imu_pub_;
  SerialCom serial_com_;
  Eigen::Quaternion<double> attitude_;
  double pitch_set_;
  double roll_set_;

  Eigen::Vector3d QuaternionToEuler(const Eigen::Quaternion<double> &q) {
  	Eigen::Vector3d euler;
  	const double w = q.w();
  	const double x = q.x();
  	const double y = q.y();
  	const double z = q.z();
  	const double pole = x*y+z*w;
  	if (pole == 0.5) {
  		euler[0] = 0;
  		euler[1] = M_PI/2;
  		euler[2] = 2*atan2(x,w);
  	}
  	else if (pole == -0.5) {
  		euler[0] = 0;
  		euler[1] = -M_PI/2;
  		euler[2] = -2*atan2(x,w);
  	}
  	else {
  		euler[0] = atan2(2*y*w-2*x*z, 1-2*y*y-2*z*z);
  		euler[1] = asin(2*x*y+2*z*w);
  		euler[2] = atan2(2*x*w-2*y*z, 1-2*x*x-2*z*z);
  	}
  	return euler;
  }

  void ImuCallback(const sensor_msgs::Imu attitude) {
  	ROS_INFO("Callback");
  	const double x = attitude.orientation.z;
  	const double y = -attitude.orientation.y;
  	const double z = attitude.orientation.x;
  	const double w = attitude.orientation.w;
  	attitude_ = Eigen::Quaternion<double>(w,x,y,z);
  	ROS_INFO("w %f x %f y %f z %f", attitude_.w(), attitude_.x(), attitude_.y(), attitude_.z());
  	Eigen::Quaternion<double> rot;
  	rot = Eigen::AngleAxis<double>(-M_PI/2, Eigen::Vector3d(1,0,0));
  	//attitude_ *= rot;	//correct weird imu cs
  	rot = Eigen::AngleAxis<double>(M_PI/2, Eigen::Vector3d(0,1,0));
  	//attitude_ *= rot;	//rotate to sensor mount orientation
  	Eigen::Vector3d euler = QuaternionToEuler(attitude_);
  	euler *= 180/M_PI;
  	ROS_INFO("roll %f pitch %f yaw %f", euler[0], euler[1], euler[2]);
  	/*geometry_msgs::PoseStamped pub;
  	pub.header = attitude.header;
  	pub.pose.orientation.x = attitude_.x();
  	pub.pose.orientation.y = attitude_.y();
  	pub.pose.orientation.z = attitude_.z();
  	pub.pose.orientation.w = attitude_.w();
  	pub.pose.position.x = 0;
  	pub.pose.position.y = 0;
  	pub.pose.position.z = 0;
  	ROS_INFO("w %f x %f y %f z %f", attitude_.w(), attitude_.x(), attitude_.y(), attitude_.z());
  	imu_pub_.publish(pub);*/
  }

};