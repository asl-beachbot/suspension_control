#include "ros/ros.h"
#include "serial_com.h"
#include "geometry_msgs/PoseStamped.h"
#include "tf/transform_datatypes.h"
#include "tf/transform_broadcaster.h"
#include "find_plane.cpp"

class SuspensionControl {
 public:

	SuspensionControl() {
		std::string addr;
		if(ros::param::get("address", addr));
		else {
			addr = "/dev/ttyUSB0"; ROS_WARN("Didn't find config for motor controller address");
		}
		serial_com_.Open(addr);
		ROS_INFO("Opened suspension controller at %s", addr.c_str());
		pose_sub_ = n_.subscribe("/localization/bot_pose", 1, &SuspensionControl::PoseCallback, this);
		pole_sub_ = n_.subscribe("/localization/beach_map", 1, &SuspensionControl::PoleCallback, this);
		find_plane_ = NULL;
	}

	~SuspensionControl() {
		serial_com_.Close();
	}
	
 private:
	ros::NodeHandle n_;
	ros::Subscriber pose_sub_;
	ros::Subscriber pole_sub_;
	SerialCom serial_com_;
	FindPlane *find_plane_;
	geometry_msgs::PoseStamped pose_;
	double pitch_set_;
	double roll_set_;

	void UpdatePlane() {
		find_plane_->CalcPlaneMin(pose_.pose);
		roll_set_ = find_plane_->GetRoll();
		pitch_set_ = find_plane_->GetPitch();
		const int roll_data = roll_set_ / M_PI * 180 * 1000;
		const int pitch_data = pitch_set_ / M_PI * 180 * 1000;
		stringstream ss;
		ss << "set roll " << roll_data << " pitch " << pitch_data;
		serial_com_.Send(ss.str() );
	}

	void PoseCallback(const geometry_msgs::PoseStamped attitude) {
		pose_ = pose;
		if (find_plane_ != NULL) UpdatePlane();
	}

	void PoleCallback(const localization::beach_map map) {
		std::vector<Pole::Line> lines;
		for (int i = 0; i < map.lines.size(); i++) {
			Pole::Line line;
			line.p.x() = map.lines[i].p.x;
			line.p.y() = map.lines[i].p.y;
			line.p.z() = map.lines[i].p.z;
			line.u.x() = map.lines[i].u.x;
			line.u.y() = map.lines[i].u.y;
			line.u.z() = map.lines[i].u.z;
			line.end.x() = map.lines[i].end.x;
			line.end.y() = map.lines[i].end.y;
			line.end.z() = map.lines[i].end.z;
			line.d = map.lines[i].d;
		}
		find_plane_ = new FindPlane(lines);
	}
};