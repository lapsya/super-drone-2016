#include <opencv2/opencv.hpp>
#include "FlashDetector.hpp"

using namespace std;
using namespace cv;


bool InEps(KeyPoint& kpt) {
	return (norm(kpt.pt - pnt.pt) < 50);
}

bool IsFlash(int& a) {

	return (a < 100 && a > 70);
}


void FlashDetector::findBlobs(InputArray _image, InputArray _binaryImage, std::vector<Center> &centers, InputArray _img) const
{
	Mat  /* image = _image.getMat(), */ binaryImage = _binaryImage.getMat(), img = _img.getMat();
	// (void)image;
	centers.clear();

	std::vector < std::vector<Point> > contours;
	Mat tmpBinaryImage = binaryImage.clone();
	findContours(tmpBinaryImage, contours, RETR_LIST, CHAIN_APPROX_NONE);

	for (size_t contourIdx = 0; contourIdx < contours.size(); contourIdx++)
	{
		Center center;
		center.confidence = 1;
		Moments moms = moments(Mat(contours[contourIdx]));

		{
			double area = moms.m00;
			if (area < minArea || area >= maxArea)
				continue;
		}

		{
			double area = moms.m00;
			double perimeter = arcLength(Mat(contours[contourIdx]), true);
			double ratio = 4 * CV_PI * area / (perimeter * perimeter);
			if (ratio < minCircularity || ratio >= maxCircularity)
				continue;
		}


		{
			double denominator = std::sqrt(std::pow(2 * moms.mu11, 2) + std::pow(moms.mu20 - moms.mu02, 2));
			const double eps = 1e-2;
			double ratio;
			if (denominator > eps)
			{
				double cosmin = (moms.mu20 - moms.mu02) / denominator;
				double sinmin = 2 * moms.mu11 / denominator;
				double cosmax = -cosmin;
				double sinmax = -sinmin;

				double imin = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmin - moms.mu11 * sinmin;
				double imax = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmax - moms.mu11 * sinmax;
				ratio = imin / imax;
			}
			else
			{
				ratio = 1;
			}

			if (ratio < minInertiaRatio || ratio >= maxInertiaRatio)
				continue;

			center.confidence = ratio * ratio;
		}


		{
			std::vector < Point > hull;
			convexHull(Mat(contours[contourIdx]), hull);
			double area = contourArea(Mat(contours[contourIdx]));
			double hullArea = contourArea(Mat(hull));
			double ratio = area / hullArea;
			if (ratio < minConvexity || ratio >= maxConvexity)
				continue;
		}

		if (moms.m00 == 0.0)
			continue;
		center.location = Point2d(moms.m10 / moms.m00, moms.m01 / moms.m00);


		{
			Vec3b colour = img.at<Vec3b>(Point(cvRound(center.location.x), cvRound(center.location.y)));
			//if (binaryImage.at<uchar>(cvRound(center.location.y), cvRound(center.location.x)) != blobColor)
			if (colour[0] < 240 && colour[1] < 240 && colour[2] < 240)
				continue;
		}

		//compute blob radius
		{
			std::vector<double> dists;
			for (size_t pointIdx = 0; pointIdx < contours[contourIdx].size(); pointIdx++)
			{
				Point2d pt = contours[contourIdx][pointIdx];
				dists.push_back(norm(center.location - pt));
			}
			std::sort(dists.begin(), dists.end());
			center.radius = (dists[(dists.size() - 1) / 2] + dists[dists.size() / 2]) / 2.;
		}

		centers.push_back(center);


	}

}

void FlashDetector::detectFlash(InputArray image, std::vector<cv::KeyPoint>& keypoints, std::vector<int>& num_flsh) {

	Mat grayscaleImage;
	if (image.channels() == 3)
		cvtColor(image, grayscaleImage, COLOR_BGR2GRAY);
	else
		grayscaleImage = image.getMat();

	if (grayscaleImage.type() != CV_8UC1) {
		CV_Error(Error::StsUnsupportedFormat, "Blob detector only supports 8-bit images!");
	}

	std::vector < std::vector<Center> > centers;
	for (double thresh = minThreshold; thresh < maxThreshold; thresh += thresholdStep)
	{
		Mat binarizedImage;
		threshold(grayscaleImage, binarizedImage, thresh, 255, THRESH_BINARY);

		std::vector < Center > curCenters;
		findBlobs(grayscaleImage, binarizedImage, curCenters, image);
		std::vector < std::vector<Center> > newCenters;
		for (size_t i = 0; i < curCenters.size(); i++)
		{
			bool isNew = true;
			for (size_t j = 0; j < centers.size(); j++)
			{
				double dist = norm(centers[j][centers[j].size() / 2].location - curCenters[i].location);
				isNew = dist >= minDistBetweenBlobs && dist >= centers[j][centers[j].size() / 2].radius && dist >= curCenters[i].radius;
				if (!isNew)
				{
					centers[j].push_back(curCenters[i]);

					size_t k = centers[j].size() - 1;
					while (k > 0 && centers[j][k].radius < centers[j][k - 1].radius)
					{
						centers[j][k] = centers[j][k - 1];
						k--;
					}
					centers[j][k] = curCenters[i];

					break;
				}
			}
			if (isNew)
				newCenters.push_back(std::vector<Center>(1, curCenters[i]));
		}
		std::copy(newCenters.begin(), newCenters.end(), std::back_inserter(centers));
	}

	for (size_t i = 0; i < centers.size(); i++)
	{
		if (centers[i].size() < minRepeatability)
			continue;
		Point2d sumPoint(0, 0);
		double normalizer = 0;
		for (size_t j = 0; j < centers[i].size(); j++)
		{
			sumPoint += centers[i][j].confidence * centers[i][j].location;
			normalizer += centers[i][j].confidence;
		}
		sumPoint *= (1. / normalizer);
		KeyPoint kpt(sumPoint, (float)(centers[i][centers[i].size() / 2].radius) * 2.0f);
		pnt = kpt;
		std::vector<KeyPoint>::iterator it = std::find_if(keypoints.begin(), keypoints.end(), InEps);
		if (it != keypoints.end()) {
			*it = kpt;
			num_flsh[it - keypoints.begin()] += 1;
			cout << num_flsh[it - keypoints.begin()] << "\n";
		}
		else {
			keypoints.push_back(kpt);
			num_flsh.push_back(1);
		}
	}
}