#include "gst_pipeline.hpp"
#include "helper.hpp"
#include "VisionPipeline.h"
#include "networktables/NetworkTable.h"

#define OUTPUT_VALUES true

int device = 0;
int width = 640;
int height = 480;
int framerate = 60;
bool mjpeg = false;
int bitrate = 600000;
int port = 5805;

int main()
{
	CvCapture_GStreamer mycam;
    string read_pipeline = createReadPipelineSplit (
            device, width, height, framerate, mjpeg, 
            bitrate, videoSinkIP, port);

	mycam.open (CV_CAP_GSTREAMER_FILE, read_pipeline.c_str());
	printf ("Succesfully opened camera with dimensions: %dx%d\n",
            width, height);

	VisionPipeline pipeline;
	
	NetworkTable::SetClientMode();
	NetworkTable::SetTeam(3674);
	NetworkTable::SetIPAddress("10.36.74.61");
	table = NetworkTable::GetTable("vision");

	
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Point2f> contour_center;
	std::vector<float> contour_angle;

	cv::Mat frame;
	cap >> frame;
	//cv::findContours(mask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	//std::cout << contours.size() << std::endl;
	for (;;)
	{
		IplImage* img = mycam.retrieveFrame(0);
		frame = cv::cvarrToMat(img).clone();
		pipeline.Process(frame);
		contours = *pipeline.GetFilterContoursOutput();
		auto t1 = Clock::now();
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

			for (unsigned int j = 0; j < 4; ++j)
				cv::line(frame, rect_points[j], rect_points[(j + 1) % 4], cv::Scalar(0, 255, 0), 2);

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
				if (!skip)
				{
					if (contour_center.at(u).x != contour_center.at(v).x && contour_center.at(u).y != contour_center.at(v).y)
					{
						if ((contour_center.at(u).y < contour_center.at(v).y + 30) && (contour_center.at(u).y > contour_center.at(v).y - 30))
						{
							if (contour_center.at(u).x < contour_center.at(v).x)
							{
								//std::cout << "swap: " << u << " " << v << std::endl;
								std::swap(contour_angle[u], contour_angle[v]);
								std::swap(contour_center[u], contour_center[v]);
							}

							for (int i = 0; i < contour_center.size(); i++)
							{
								//std::cout << " u " << u << "  " << contour_center.at(u).x << " i " << i << "  " << contour_center.at(i).x << " v " << v << "  " << contour_center.at(v).x << std::endl;

								if (contour_center.at(u).x > contour_center.at(i).x && contour_center.at(v).x < contour_center.at(i).x)
								{
									skip_ = true;
									//std::cout << "break" << std::endl;
									break;
								}
								else
								{
									skip_ = false;
								}

							}
							if (!skip_)
							{
								//std::cout << "pos: " << u << "  " << v << " angle: " << contour_angle.at(u) << "  " << contour_angle.at(v) << std::endl;
								if ((contour_angle.at(v) > -3 && contour_angle.at(v) < 20 && contour_angle.at(u) < 3 && contour_angle.at(u) > -20))
								{
									cv::circle(frame, cv::Point((contour_center.at(u).x + contour_center.at(v).x) / 2, (contour_center.at(u).y + contour_center.at(v).y) / 2), 5, cv::Scalar(255, 0, 0), 2);
								}
							}
						}
					}
				}
			}
		}
		auto t2 = Clock::now();

		std::cout << "Delta t2-t1: "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
			<< " milliseconds" << std::endl;
		cv::waitKey();

		cv::imshow("mask", frame);
		contour_angle.clear();
		contour_center.clear();
		contours.clear();
	}
	return 0;
}