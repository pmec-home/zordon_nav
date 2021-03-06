#include "ros/ros.h"
#include <stdlib.h>
#include <time.h>
#include <sensor_msgs/Imu.h>
#include <nav_msgs/Odometry.h>
#include <math.h>


const std::string PREFIX_MSG = "Zordon";

nav_msgs::Odometry odometry_msg;

const std::string odometry_msg_name = PREFIX_MSG + "/wheels_odom";
const std::string odometry_msg_child_frame_id = PREFIX_MSG + "_base";
const std::string odometry_msg_frame_id = PREFIX_MSG + "_odom";

const std::string cmd_vel_msg_name = PREFIX_MSG + "/cmd_vel";



struct Velocity {
	double linear;
	double angular;
};

struct Imu {
	double x;
	double y;
	double z;
	double w;
};

struct Pose {
	double x;
	double y;
	double theta;
};

struct Velocity baseVelocity = {0,0};
struct Pose basePose = {0,0,0};
struct Imu imu = {0,0,0,0};


float randomNumber(float Min, float Max)
{
    return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
}

void commandVelocityCallback(const geometry_msgs::Twist& msg)
{
	baseVelocity.linear = msg.linear.x;
	baseVelocity.angular = msg.angular.z;	
}

void commandImuCallback(const sensor_msgs::Imu& msg) {
	imu.x = msg.orientation.x;
	imu.y = msg.orientation.y;
	imu.z = msg.orientation.z;
	imu.w = msg.orientation.w;
}

int main(int argc, char **argv)
{
	srand (time(NULL));	

	ros::init(argc, argv, "virtual_odom");
	ros::NodeHandle n;
	ros::Publisher odometry_msg_pub = n.advertise<nav_msgs::Odometry>("Zordon/wheels_odom", 1);
	ros::Subscriber cmd_vel_sub = n.subscribe("Zordon/cmd_vel", 1000, &commandVelocityCallback);
	ros::Subscriber cmd_imu_sub = n.subscribe("Zordon/imu", 1000, &commandImuCallback);
	ros::Rate loop_rate(200);

	odometry_msg.header.frame_id = "Zordon_odom";
	odometry_msg.child_frame_id = "Zordon_base";
	
	double diagonal_value = 0.01;
	for(int i = 0; i < 36; i++) {
		if(i == 0 || i == 7 || i == 14 ||
			i == 21 || i == 28 || i == 35)
			odometry_msg.twist.covariance[i] = diagonal_value;
		else
			odometry_msg.twist.covariance[i] = 0;
	}



	double time_last = 0;	
	double last_theta = 0;
	double theta;
	while (ros::ok()) {
		theta = atan2(2*(imu.w * imu.z + imu.x*imu.y), (1-2*(imu.y*imu.y+imu.z*imu.z)));	
	
		double time_now = ros::Time::now().toSec();
		double time_elapsed = time_now - time_last;
		time_last = time_now;
		double error_linear = randomNumber(-0.0005, 0.0005);
		double error_angular = randomNumber(-0.002, 0.002);	
		double linear = 0;
		double angular = 0;
		if(baseVelocity.linear != 0.0)
			linear = baseVelocity.linear + error_linear;	
		if(baseVelocity.angular != 0.0)
			angular = baseVelocity.angular + error_angular;
		
		basePose.x = basePose.x + time_elapsed * linear * cos(theta);
		basePose.y = basePose.y + time_elapsed * linear * sin(theta);
		basePose.theta = basePose.theta + time_elapsed * angular;

		odometry_msg.twist.twist.linear.x = linear;
		odometry_msg.twist.twist.angular.z = angular;
		odometry_msg.twist.twist.linear.y = 0; //baseVelocity.linear;
		odometry_msg.twist.twist.linear.z = 0; //randomNumber(-0.005, 0.005);

		odometry_msg.pose.pose.position.x = basePose.x;
		odometry_msg.pose.pose.position.y = basePose.y;
		odometry_msg.pose.pose.position.z = 0.0;//0.0;//pose.theta *180/3.1415;

		odometry_msg.pose.pose.orientation.x = 0.0;//goal_vel.angular[2];
		odometry_msg.pose.pose.orientation.y = 0.0;//dxl_wb.itemRead(DXL_ID, "Present_Position");
		odometry_msg.pose.pose.orientation.z =  imu.z;//*/sin(basePose.theta/2.0);
		odometry_msg.pose.pose.orientation.w = imu.w;//*/cos(basePose.theta/2.0);
		
		odometry_msg.header.stamp = ros::Time::now();

		odometry_msg_pub.publish(odometry_msg);

		last_theta = theta;

		ros::spinOnce();
		loop_rate.sleep();
	}
	return 0;
}
