#include "ros/ros.h"
#include "serial_com.h"
#include "sensor_msgs/Imu.h"
#include "geometry_msgs/PoseStamped.h"
#include <Eigen/Geometry>
#include <cmath>
#include "tf/transform_datatypes.h"
#include "tf/transform_broadcaster.h"

class SuspensionControl {
 public:

  SuspensionControl() {
    const std::string addr = "/dev/ttyUSB0";
    //serial_com_.Open(addr);
    ROS_INFO("Opened suspension controller at %s", addr.c_str());

    const std::string topic = "/imu/data";
    imu_sub_ = n_.subscribe(topic, 1, &SuspensionControl::ImuCallback, this);
    ROS_INFO("Suscribe to IMU topic \"%s\"", topic.c_str());

    LoadParameters();
  }

  ~SuspensionControl() {
    //delete &serial_com_;
  }
  
 private:
  ros::NodeHandle n_;
  ros::Subscriber imu_sub_;
  SerialCom serial_com_;
  Eigen::Quaternion<double> attitude_;
  double pitch_set_;
  double roll_set_;
  bool imu_mounted_;
  ros::Time time_;

  void PublishTf() {
		static tf::TransformBroadcaster br;
		tf::Transform transform;
		transform.setOrigin( tf::Vector3(0.0, 0.0, 0.0));
		transform.setRotation(tf::Quaternion(attitude_.x(), attitude_.y(), attitude_.z(), attitude_.w()));
		br.sendTransform(tf::StampedTransform(transform, time_, "fixed_frame", "laser_frame"));
	}

  double GetYaw() {
    sensor_msgs::Imu temp;
    temp.orientation.x = attitude_.x();
    temp.orientation.y = attitude_.y();
    temp.orientation.z = attitude_.z();
    temp.orientation.w = attitude_.w();
    return tf::getYaw(temp.orientation);
  }

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

  void LoadParameters() {
  	if (ros::param::get("imu_mounted", imu_mounted_));
  	else {
  		ROS_INFO("No config for imu_mounted. Setting true");
  		imu_mounted_ = true;
  	}
  }

  void ImuCallback(const sensor_msgs::Imu attitude) {
  	//ROS_INFO("Callback");
  	const double x = attitude.orientation.x;
  	const double y = attitude.orientation.y;
  	const double z = attitude.orientation.z;
  	const double w = attitude.orientation.w;
  	attitude_ = Eigen::Quaternion<double>(w,x,y,z);
  	Eigen::Quaternion<double> rot;
  	rot = Eigen::AngleAxis<double>(M_PI/2, Eigen::Vector3d(0,0,1));
  	attitude_ *= rot;	//correct weird imu cs
	  if (imu_mounted_) {	
	  	rot = Eigen::AngleAxis<double>(M_PI/2, Eigen::Vector3d(0,1,0));
	  	attitude_ *= rot;	//rotate to sensor mount orientation	DISABLED FOR TESTING
	  }
  	time_ = attitude.header.stamp;
    //ROS_INFO("yaw: %f", GetYaw()/M_PI*180);
  	PublishTf();	//publish new transform
  }
};