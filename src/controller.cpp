#include "suspension_control.h"
#include "ros/ros.h"

int main(int argc, char** argv) {
  ros::init(argc, argv, "suspension_control");
  SuspensionControl *suspension_control = new SuspensionControl();
  while(ros::ok()) {
    ros::spin();
  }
  delete suspension_control;
}