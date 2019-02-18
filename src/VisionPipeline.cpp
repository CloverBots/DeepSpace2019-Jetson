#include "VisionPipeline.h"
#include <vector>

VisionPipeline::VisionPipeline(){};

std::vector<std::vector<cv::Point> > VisionPipeline::Process(cv::Mat& source0)
{
	cv::Mat input = source0;
	double hslThresholdHue[] = { 0.0, 40.0 };
	double hslThresholdSaturation[] = { 0.0, 70.0 };
	double hslThresholdLuminance[] = { 250.0, 255.0 };
	cv::cvtColor(input, input, cv::COLOR_BGR2HLS);
	//cv::imshow("idk", input);
	//cv::waitKey();
	cv::inRange(input, cv::Scalar(hslThresholdHue[0], hslThresholdLuminance[0], hslThresholdSaturation[0]), 
					 cv::Scalar(hslThresholdHue[1], hslThresholdLuminance[1], hslThresholdSaturation[1]), input);
	//cv::imshow("idk", input);
	//cv::waitKey();

	//cv::cvtColor(input, input, cv::COLOR_HLS2BGR);
	//imshow("a", input);
	//cv::waitKey(0);	
	cv::Point cvErodeAnchor(-1, -1);
	double cvErodeIterations = 0.0;
	cv::Mat cvErodeKernel;
 	int cvErodeBordertype = cv::BORDER_CONSTANT;
	cv::Scalar cvErodeBordervalue(-1);
	cv::erode(input, input, cvErodeKernel, cvErodeAnchor, (int)cvErodeIterations, cvErodeBordertype, cvErodeBordervalue);
	cv::Mat cvDilateKernel;
	cv::Point cvDilateAnchor(-1, -1);
	double cvDilateIterations = 5.0;  // default Double
    	int cvDilateBordertype = cv::BORDER_CONSTANT;
	cv::Scalar cvDilateBordervalue(-1);
	cv::dilate(input, input, cvDilateKernel, cvDilateAnchor, (int)cvDilateIterations, cvDilateBordertype, cvDilateBordervalue);
	//	cv::imshow("idk", input);
	//cv::waitKey();
	std::vector<std::vector<cv::Point> > contours;
	bool findContoursExternalOnly = false;
	std::vector<cv::Vec4i> hierarchy;
	contours.clear();
	int mode = findContoursExternalOnly ? cv::RETR_EXTERNAL : cv::RETR_LIST;
	int method = cv::CHAIN_APPROX_SIMPLE;
	cv::findContours(input, contours, hierarchy, mode, method);
	double filterContoursMinArea = 10.0;
	double filterContoursMinPerimeter = 0; 
	double filterContoursMinWidth = 0;
	double filterContoursMaxWidth = 1000;
	double filterContoursMinHeight = 0;
	double filterContoursMaxHeight = 1000;
	double filterContoursSolidity[] = {0, 1000};
	double filterContoursMaxVertices = 1000000;
	double filterContoursMinVertices = 0;
	double filterContoursMinRatio = 0;
	double filterContoursMaxRatio = 1000;
	std::vector<cv::Point> hull;
	std::vector<std::vector<cv::Point> > output;
	output.clear();
	for (std::vector<cv::Point> contour: contours) {
		cv::Rect bb = boundingRect(contour);
		if (bb.width < filterContoursMinWidth || bb.width > filterContoursMaxWidth) continue;
		if (bb.height < filterContoursMinHeight || bb.height > filterContoursMaxHeight) continue;
		double area = cv::contourArea(contour);
		if (area < filterContoursMinArea) continue;
		if (arcLength(contour, true) < filterContoursMinPerimeter) continue;
		cv::convexHull(cv::Mat(contour, true), hull);
		double solid = 100 * area / cv::contourArea(hull);
		if (solid < filterContoursSolidity[0] || solid > filterContoursSolidity[1]) continue;
		if (contour.size() < filterContoursMinVertices || contour.size() > filterContoursMaxVertices)	continue;
		double ratio = (double) bb.width / (double) bb.height;
		if (ratio < filterContoursMinRatio || ratio > filterContoursMaxRatio) continue;
		output.push_back(contour);
	}
	return output;
}
