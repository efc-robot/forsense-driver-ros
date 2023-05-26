// serial_imu.cpp
#include "serial_parse.h"
#include <array>

#define BUF_SIZE 1024

ros::Publisher IMU_pub; // IMU话题发布类
ros::Publisher MAG_pub; // MAG话题发布类

sensor_msgs::Imu imu_msg;
sensor_msgs::MagneticField mag_msg;

// uint8_t rate_cmd[34];

std::array<uint8_t, 34> rate_cmd;

// 串口数据缓存

// 存储节点参数
static int frame_rate;
static int frame_count;
std::string imu_topic;
std::string mag_topic;
std::string imu_frame;
std::string mag_frame;
std::string imu_div_name;
std::string ns = "/";
static int imu_div_buad = 460800;
int sample_rate = 100;
int imu_buf_size;

static uint8_t buf[2048];

void timer(int sig);

void get_cmd_by_rate(int rate, std::array<uint8_t, 34> &cmd);

int main(int argc, char **argv)
{
	int rev = 0;
	ros::init(argc, argv, "serial_imu");
	// ros::NodeHandle imu_node_handle;
	ros::NodeHandle imu_node_handle("~");

	imu_node_handle.param<std::string>("imu_div_name", imu_div_name, "/dev/ttyUSB0");
	imu_node_handle.param<std::string>("imu_topic", imu_topic, "imu_raw");
	imu_node_handle.param<std::string>("mag_topic", mag_topic, "mag_raw");
	imu_node_handle.param<std::string>("imu_frame", imu_frame, "imu");
	imu_node_handle.param<int>("sample_rate", sample_rate, 100);
	imu_node_handle.param<std::string>("ns", ns, "/");

	imu_frame = ns.compare("/") ? ns + "/" + imu_frame : imu_frame;

	imu_msg.header.frame_id = imu_frame;
	mag_msg.header.frame_id = imu_frame;

	ROS_INFO_STREAM("imu_div_name:" << imu_div_name);
	ROS_INFO_STREAM("imu_div_buad:" << imu_div_buad);
	ROS_INFO_STREAM("imu_topic:" << imu_topic);
	ROS_INFO_STREAM("mag_topic:" << mag_topic);
	ROS_INFO_STREAM("imu_frame:" << imu_frame);

	IMU_pub = imu_node_handle.advertise<sensor_msgs::Imu>(imu_topic, 20);
	MAG_pub = imu_node_handle.advertise<sensor_msgs::MagneticField>(mag_topic, 20);

	serial::Serial sp;

	serial::Timeout to = serial::Timeout::simpleTimeout(100);

	sp.setPort(imu_div_name);

	sp.setBaudrate(imu_div_buad);

	sp.setTimeout(to);

	signal(SIGALRM, timer);

	try
	{
		sp.open();
	}
	catch (serial::IOException &e)
	{
		ROS_ERROR_STREAM("Unable to open port.");
		return -1;
	}

	if (sp.isOpen())
	{
		ROS_INFO_STREAM("imu port: " + imu_div_name + " is opened.");
	}
	else
	{
		return -1;
	}

	alarm(1);

	ros::Rate loop_rate(500);

	get_cmd_by_rate(sample_rate, rate_cmd);

	for (int i = 0; i < 10; i++)
		sp.write(rate_cmd.data(), rate_cmd.size());

	// start
	uint8_t cmd[34]={0x55,0xaa,0x03,0x00,0x18,0x00,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x52,0xd8,0x8e,0xe8};

	for(int i=0;i<10;i++)
		sp.write(cmd, sizeof(cmd));

	while (ros::ok())
	{
		size_t num = sp.available();
		if (num != 0)
		{
			uint8_t buffer[BUF_SIZE];

			if (num > BUF_SIZE)
				num = BUF_SIZE;

			num = sp.read(buffer, num);
			if (num > 0)
			{

				for (int i = 0; i < num; i++)
				{
					imu_rx(buffer[i]);
				}
			}
		}
		loop_rate.sleep();
	}

	sp.close();

	return 0;
}

void timer(int sig)
{
	if (SIGALRM == sig)
	{
		frame_rate = frame_count;
		frame_count = 0;
		alarm(1);
	}
}

void get_cmd_by_rate(int rate, std::array<uint8_t, 34> &cmd)
{
	switch (rate)
	{
	case 400:
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0xc8,
			   0x43, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x94, 0xa8, 0xf6, 0x12};
		break;
	case 200:
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0x48,
			   0x43, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x5b, 0xe9, 0x1b, 0x5d};
		break;
	case 100:
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0xc8,
			   0x42, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x0a, 0x2b, 0x2c, 0x8d};
		break;
	case 50:
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0x48,
			   0x42, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0xc5, 0x6a, 0xc1, 0xc2};
		break;
	case 10:
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0x20,
			   0x41, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x22, 0x7f, 0x4d, 0x42};
		break;
	case 1:
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0x80,
			   0x3f, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x5b, 0xda, 0x3d, 0x7e};
		break;
	default: // 100hz
		cmd = {0x55, 0xaa, 0x0e, 0x00, 0x18, 0x00, 0x00, 0x00, 0xc8,
			   0x42, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x00, 0x00, 0x00, 0x00, 0x0a, 0x2b, 0x2c, 0x8d};
	}
}