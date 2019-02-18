#include "gst_pipeline.hpp"
#include "helper.hpp"
#include "VisionPipeline.h"
#include "networktables/NetworkTable.h"
#include <chrono>
#include <fstream>

typedef std::chrono::high_resolution_clock Clock;

#define OUTPUT_VALUES true

const int angle_min = 3;
const int angle_max = 40;

int device = 0;
int width = 1280;
int height = 720;
int framerate = 60;
bool mjpeg = false;
int bitrate = 600000;
int port = 5805;
std::string videoSinkIP = "10.36.74.2";

int closest(std::vector<cv::Point2f> vec, int value)
{
	std::vector<int> distances;
	int Closest = value;
	int val = value;
	for(int i = 0; i < vec.size(); i++)
	{	
		distances.push_back(abs(value - vec[i].x));
		std::cout << abs(value - vec[i].x) << "  " << i << std::endl;
	}
	if(vec.size() != 0)
	{	
		Closest = 10000;
	}
	for(int i = 0; i < distances.size(); i++)
	{
		if(distances.at(i) < Closest)
		{
			//std::cout << "closest  "<< distances.at(i) <<"  " << i << std::endl;  
			Closest = distances.at(i);
			val = i;
		}
	}
	
	//std::cout << "closest val "<< distances.at(val) << std::endl;
	
	if(vec.size() != 0)
	{	
		return vec[val].x;
	}
	else
	{
		return 1280/2;
	}
}

int main()
{
	std::ofstream file {"file.txt"};
	CvCapture_GStreamer mycam;
	std::cout << "starting pipeline" << std::endl;
    	string read_pipeline = createReadPipelineSplit (
            device, width, height, framerate, mjpeg, 
            bitrate, videoSinkIP, port);
	while(true)
	{
		std::cout << "restarting pipeline" << std::endl;
		if(mycam.open (CV_CAP_GSTREAMER_FILE, read_pipeline.c_str()))
		{
			break;
		}
	}
	printf ("Succesfully opened camera with dimensions: %dx%d\n",
            width, height);

	VisionPipeline pipeline;
	std::shared_ptr<NetworkTable> table;
	NetworkTable::SetClientMode();
	NetworkTable::SetTeam(3674);
	NetworkTable::SetIPAddress("10.36.74.2");
	table = NetworkTable::GetTable("vision");

	
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Point2f> contour_center;
	std::vector<cv::Point2f> data_center;
	std:vector<int> data_size;
	std::vector<float> contour_angle;
	int lost_frames;
	int prev_data;
	cv::Mat frame;
	int tempx;
	int tempy;
	//cv::findContours(mask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	//std::cout << contours.size() << std::endl;
	for (;;)
	{
		auto t1 = Clock::now();
		mycam.grabFrame();
		IplImage* img = mycam.retrieveFrame(0);

		frame = cv::cvarrToMat(img).clone();
		//cv::imshow("idk", frame);
		//cv::waitKey();
		contours = pipeline.Process(frame);
		//std::cout << contours.size() << std::endl;
		for (int i = 0; i < contours.size(); ++i)
		{
			cv::RotatedRect rotatedRect = cv::minAreaRect(contours[i]);

			cv::Point2f rect_points[4];
			rotatedRect.points(rect_points);

			cv::Point2f edge1 = cv::Vec2f(rect_points[1].x, rect_points[1].y) - cv::Vec2f(rect_points[0].x, rect_points[0].y);
			cv::Point2f edge2 = cv::Vec2f(rect_points[2].x, rect_points[2].y) - cv::Vec2f(rect_points[1].x, rect_points[1].y);


			cv::Point2f usedEdge = edge1;
			if (cv::norm(edge2) < cv::norm(edge1))
				usedEdge = edge2;

			cv::Point2f reference = cv::Vec2f(1, 0);


			float angle = 180.0f / CV_PI * acos((reference.x*usedEdge.x + reference.y*usedEdge.y) / (cv::norm(reference) *cv::norm(usedEdge)));
			cv::Point2f center = rotatedRect.center;

			contour_center.push_back(center);
			if (angle > 90)
			{
				angle = -(180 - angle);
			}
			angle = -angle;
			contour_angle.push_back(angle);
			//std::cout << angle << std::endl;
			for (unsigned int j = 0; j < 4; ++j)
				cv::line(frame, rect_points[j], rect_points[(j + 1) % 4], cv::Scalar(0, 0, 255), 2);

			std::stringstream ss;
			ss << angle << ", " << i;
			cv::putText(frame, ss.str(), center + cv::Point2f(-25, 25), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 0, 255)); // print angle
		}
		std::vector<cv::Point> used;
		bool skip = false;
		bool skip_ = false;
		for (int u = 0; u < contour_center.size(); u++)
		{
			for (int v = 0; v < contour_center.size(); v++)
			{

				used.push_back(cv::Point(u, v));
				for (int i = 0; i < used.size(); i++)
				{
					if ((used.at(i).x == v && used.at(i).y == u))
					{
						skip = true;

						break;
					}
					else
					{
						skip = false;
					}
				}		
				//std::cout << u << "   " << v << std::endl;
				if (!skip)
				{
					if (contour_center.at(u).x != contour_center.at(v).x && contour_center.at(u).y != contour_center.at(v).y)
					{
						//std::cout << "contour not self" << std::endl;
						//if ((contour_center.at(u).y < contour_center.at(v).y + 30) && (contour_center.at(u).y > contour_center.at(v).y - 30))
						//{
							if (contour_center.at(u).x < contour_center.at(v).x)
							{
								//std::cout << "swap: " << u << " " << v << std::endl;
								std::swap(contour_angle[u], contour_angle[v]);
								std::swap(contour_center[u], contour_center[v]);
							}

							for (int i = 0; i < contour_center.size(); i++)
							{
								
								if (contour_center.at(u).x > contour_center.at(i).x && contour_center.at(v).x < contour_center.at(i).x)
								{
									//std::cout << i << "   "<< u <<"   "<<v<< std::endl;
											
									if ((contour_angle.at(i) > angle_min && contour_angle.at(i) < angle_max) || (contour_angle.at(i) < -angle_min && contour_angle.at(i) > -angle_max))
									{

										//cv::circle(frame, cv::Point(contour_center.at(i).x, contour_center.at(i).y), 10, cv::Scalar(0, 0, 0), 2);
										//std::cout << contour_angle.at(i) <<std::endl;
										skip_ = true;
										//std::cout << "break" << std::endl;
										break;
									}
									
									tempx = contour_center.at(i).x;
									tempy = contour_center.at(i).y;
								}
								else
								{
									//std::cout << u << "   " << v << std::endl;
									tempx = 0;
									tempy = 0;
									skip_ = false;
								}

							}
							//std::cout << tempx << "   " << tempy << std::endl; 
							//std::cout << "end frame" << std::endl;
							if (!skip_)
							{
								if ((contour_angle.at(v) > angle_min && contour_angle.at(v) < angle_max && contour_angle.at(u) < -angle_min && contour_angle.at(u) > -angle_max))
								{
									//std::cout<< "contour found" << std::endl;
									data_center.push_back(cv::Point2f((contour_center.at(u).x + contour_center.at(v).x) / 2,
										 (contour_center.at(u).y + contour_center.at	(v).y) / 2));
									data_size.push_back(contour_center.at(u).x - contour_center.at(v).x);
							cv::circle(frame, cv::Point(tempx,tempy), 5, cv::Scalar(255, 255, 255), 2);
										
									cv::circle(frame, cv::Point((contour_center.at(u).x + contour_center.at(v).x) / 2,
										 (contour_center.at(u).y + contour_center.at	(v).y) / 2), 5, cv::Scalar(255, 0, 0), 2);
								}
							}
						//}
					}
				}
			}
			
		}
		cv::circle(frame, cv::Point(closest(data_center, 1280/2),720/2), 7, cv::Scalar(255, 255, 255), 4);
		cv::circle(frame, cv::Point(1280/2,720/2), 7, cv::Scalar(255, 0, 255), 4);
		auto t2 = Clock::now();
		/*if(data_center.size() < 0)
		{
			lost_frames++;
		}
		if(data_center.size() > 0 && lost_frames > 0)
		{
			lost_frames = 0;
		}
		if(lost_frames == 0)
		{
			table->PutNumber("data_amount", data_center.size());
			prev_data = data_center.size();
		}
		else if(lost_frames > 0)
		{
			table->PutNumber("data_amount", data_center.size());
		}
		else if(lost_frames > 10)
		{
			table->PutNumber("data_amount", 0);
		}
		else
		{}*/
			
		//std::cout << "Delta t2-t1: "
		//	<< std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
		//	<< " milliseconds" << std::endl;
		//std::cout << contour_center.at(0).x << std::endl;
		table->PutNumber("data_amount", data_center.size());
		
		for(int i = 0; i < data_center.size(); i++)
		{
			//std::cout<<data_center.at(i).y<<std::endl;
			table->PutNumber("data_center"+i, data_center.at(i).x);
			table->PutNumber("data_size"+i, data_size.at(i));
		}
		//cv::imshow("mask", frame);
		//cv::waitKey();
		contour_angle.clear();
		contour_center.clear();
		data_size.clear();
		data_center.clear();
		contours.clear();
	}
	return 0;
}
