#include "ros/ros.h"
#include "visualization_msgs/Marker.h"
#include <cassert>
#include <Eigen/Dense>
#include <iostream>
#include "tf/transform_datatypes.h"
#include "localization/pole.h"

class FindPlane {
 public:

  void CalcAnglesMin(const geometry_msgs::Pose &pose) {
  	Eigen::VectorXd l(n_poles_), beta(n_poles_), w(n_poles_), alpha_t(n_poles_), c(2);
		const double pose_x = pose.position.x; const double pose_y = pose.position.y; const double pose_z = 0.0;
		const double pose_th = tf::getYaw(pose.orientation);
  	for (int i = 0; i < n_poles_; i++) {
  		const double px = lines_[i].p.x(); const double py = lines_[i].p.y(); const double pz = lines_[i].p.z(); 
  		const double endx = lines_[i].end.x(); const double endy = lines_[i].end.y(); const double endz = lines_[i].end.z(); 
  		l(i) = sqrt((px - pose_x) * (px - pose_x) + (py - pose_y) * (py - pose_y));
  		beta(i) = atan2(py - pose_y, px - pose_x) - pose_th;
  		NormalizeAngle(beta(i));
  		alpha_t(i) = atan2(endz - laser_height, l(i)) - atan2(laser_height - pz, l(i)) / 2;	//calc target angle
  	}
  	Eigen::MatrixXd A = Eigen::MatrixXd::Zero(2, 2);
  	c = Eigen::Vector2d::Zero();
  	const double l_max = l.maxCoeff();
  	for (int i = 0; i < n_poles_; i++) {	//second loop because l_max needs to be known
  		w(i) = 1 * (l(i) / l_max) * (l(i) / l_max);	//punish for being close
  		if (std::abs(beta(i)) > M_PI*3/4) w(i) *= punish;	//punish if pole is not visible
  		A(0,0) += 2 * sin(beta(i)) * sin(beta(i)) * w(i); 
  		A(0,1) += 2 * sin(beta(i)) * cos(beta(i)) * w(i);
  		A(1,0) += 2 * cos(beta(i)) * sin(beta(i)) * w(i); 
  		A(1,1) += 2 * cos(beta(i)) * cos(beta(i)) * w(i);
  		c(0) += 2 * sin(beta(i)) * alpha_t(i) * w(i);
  		c(1) += 2 * cos(beta(i)) * alpha_t(i) * w(i);
  	} 
  	Eigen::Vector2d solution = A.colPivHouseholderQr().solve(c);	//QR-solve 
  	ROS_INFO("roll %f pitch %f", solution.x(), solution.y());
  	roll_ = solution.x(); pitch_ = solution.y();	
  	PublishMarker(l, beta);
  }

  void CalcAnglesOpt(const geometry_msgs::Pose &pose) {
    Eigen::VectorXd l(n_poles_), beta(n_poles_), w(n_poles_), alpha_t(n_poles_), c(n_poles_);
    const double pose_x = pose.position.x; const double pose_y = pose.position.y; const double pose_z = 0.0;
    const double pose_th = tf::getYaw(pose.orientation);
    for (int i = 0; i < n_poles_; i++) {
      const double px = lines_[i].p.x(); const double py = lines_[i].p.y(); const double pz = lines_[i].p.z(); 
      const double endx = lines_[i].end.x(); const double endy = lines_[i].end.y(); const double endz = lines_[i].end.z(); 
      l(i) = sqrt((px - pose_x) * (px - pose_x) + (py - pose_y) * (py - pose_y));
      beta(i) = atan2(py - pose_y, px - pose_x) - pose_th;
      NormalizeAngle(beta(i));
      alpha_t(i) = atan2(endz - laser_height, l(i)) - atan2(laser_height - pz, l(i)) / 2; //calc target angle
    }
    Eigen::MatrixXd A(n_poles_, 2);
    const double l_max = l.maxCoeff();
    for (int i = 0; i < n_poles_; i++) {  //second loop because l_max needs to be known
      w(i) = 1 * (l(i) / l_max) * (l(i) / l_max); //punish for being close
      if (std::abs(beta(i)) > M_PI*3/4) w(i) *= punish; //punish if pole is not visible
      A(i,0) = sin(beta(i)) * w(i); A(i,1) = cos(beta(i)) * w(i);
      c(i) = alpha_t(i) * w(i);
    } 
    Eigen::Vector2d solution = A.colPivHouseholderQr().solve(c);  //QR-solve 
    ROS_INFO("roll %f pitch %f", solution.x(), solution.y());
    roll_ = solution.x(); pitch_ = solution.y();  
    PublishMarker(l, beta);
  }

  double GetRoll() {
  	return roll_;
  }

  double GetPitch() {
  	return pitch_;
  }
	
  FindPlane(const std::vector<Pole::Line> &lines) {
  	lines_ = lines;
  	n_poles_ = lines.size();
  	marker_pub_ = n_.advertise<visualization_msgs::Marker>("/lines", 10, true);
  	geometry_msgs::Pose pose;
  	pose.position.x = 0;
  	pose.position.y = 0;
  	pose.orientation = tf::createQuaternionMsgFromYaw(0/180.0*M_PI);
  	CalcAnglesMin(pose);
  }

 private:

  std::vector<Pole::Line> lines_;
  int n_poles_;
  static const double laser_height = 0.35;	//height of laser above ground
  static const double punish = 0.1;		//factor by which to punish invisible poles
  double pitch_, roll_;
  ros::NodeHandle n_;
  ros::Publisher marker_pub_;

  void PublishMarker(const Eigen::VectorXd &l, const Eigen::VectorXd &beta) {
    visualization_msgs::Marker points, cube;
    points.header.stamp = cube.header.stamp = ros::Time::now();
    points.header.frame_id = cube.header.frame_id = "robot_frame";
    points.ns = "points_and_lines";
    cube.ns = "planes";
    points.action = cube.action = visualization_msgs::Marker::ADD;
    points.pose.orientation.w = 1.0;
    points.id = 1;
    cube.id = 0;
    points.type = visualization_msgs::Marker::POINTS;
    cube.type = visualization_msgs::Marker::CUBE;
    double xmax = -2000, xmin = 2000, ymax = -2000, ymin = 2000;
    for (int i = 0; i < n_poles_; i++) {
    	if (std::abs(lines_[i].p.x()) > xmax) xmax = std::abs(lines_[i].p.x());
    	if (std::abs(lines_[i].p.y()) > ymax) ymax = std::abs(lines_[i].p.y());
    }
    cube.pose.position.x = 0; cube.pose.position.y = 0; cube.pose.position.z = laser_height; 
    cube.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(roll_, -pitch_, 0);
    cube.scale.x = xmax * 2 + 1; 
    cube.scale.y = ymax * 2 + 1;
    cube.scale.z = 0.0;
    cube.color.g = 1.0f;
    cube.color.r = 1.0f;
    cube.color.a = 0.3;
    points.scale.x = 0.1;
    points.scale.y = 0.1;
    points.color.g = 1.0f;
    points.color.r = 1.0f;
    points.color.a = 1.0;
    for (int i = 0; i < l.size(); i++) {
    	geometry_msgs::Point point;
    	point.x = l(i) * cos(beta(i)); 
    	point.y = l(i) * sin(beta(i)); 
    	point.z = tan(cos(beta(i)) * pitch_ + sin(beta(i)) * roll_) * l(i) + laser_height;
    	points.points.push_back(point);
    }
    marker_pub_.publish(points);
    marker_pub_.publish(cube);
  }

  void NormalizeAngle(double& angle) {
  while(angle > M_PI && ros::ok()) angle -= 2*M_PI;
  while(angle < -M_PI && ros::ok()) angle += 2*M_PI;
	}
};
