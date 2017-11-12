#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <string>
#include <stdio.h>
#include <conio.h>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <MMSystem.h>
#define MS_NO_COREDLL
#include <iostream>


using namespace std;

cv::Point centerIrisRight;
cv::Point centerIrisLeft;
cv::Point centerIrisCalibR;
cv::Point centerIrisCalibL;
cv::Point leftEyeTL;
cv::Point rightEyeTL;
cv::Point rightEyeCalibTL;
cv::Point leftEyeCalibTL;

cv::Point centerEyeFinalR;
cv::Point centerEyeFinalL;

cv::Point leftEyeCalibFinal;
cv::Point rightEyeCalibFinal;
bool iris = true;
bool firstClick = false;
bool calibration = false;
int counter = 0;



void singleLMB(POINT pos_cursor)
{
	mouse_event(MOUSEEVENTF_LEFTDOWN, pos_cursor.x, pos_cursor.y, 0, 0); \
		PlaySound(TEXT("mouseclick.wav"), NULL, SND_FILENAME);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	mouse_event(MOUSEEVENTF_LEFTUP, pos_cursor.x, pos_cursor.y, 0, 0);

}


cv::Vec3f getEyeball(cv::Mat &eye, std::vector<cv::Vec3f> &circles)
{
	std::vector<int> sums(circles.size(), 0);
	for (int y = 0; y < eye.rows; y++)
	{
		uchar *ptr = eye.ptr<uchar>(y);
		for (int x = 0; x < eye.cols; x++)
		{
			int value = static_cast<int>(*ptr);
			for (int i = 0; i < circles.size(); i++)
			{
				cv::Point center((int)std::round(circles[i][0]), (int)std::round(circles[i][1]));
				int radius = (int)std::round(circles[i][2]);
				if (std::pow(x - center.x, 2) + std::pow(y - center.y, 2) < std::pow(radius, 2))
				{
					sums[i] += value;
				}
			}
			++ptr;
		}
	}
	int smallestSum = 9999999;
	int smallestSumIndex = -1;
	for (int i = 0; i < circles.size(); i++)
	{
		if (sums[i] < smallestSum)
		{
			smallestSum = sums[i];
			smallestSumIndex = i;
		}
	}
	return circles[smallestSumIndex];
}

cv::Rect getLeftmostEye(std::vector<cv::Rect> &eyes)
{
	int leftmost = 99999999;
	int leftmostIndex = -1;
	for (int i = 0; i < eyes.size(); i++)
	{
		if (eyes[i].tl().x < leftmost)
		{
			leftmost = eyes[i].tl().x;
			leftmostIndex = i;
		}
	}
	//leftEyeTL = eyes[leftmostIndex].tl;
	return eyes[leftmostIndex];
}


cv::Rect getRightmostEye(std::vector<cv::Rect> &eyes)
{
	int rightmost = 0;
	int rightmostIndex = -1;
	for (int i = 0; i < eyes.size(); i++)
	{
		if (eyes[i].tl().x > rightmost)
		{
			rightmost = eyes[i].tl().x;
			rightmostIndex = i;
		}
	}
	return eyes[rightmostIndex];
}

std::vector<cv::Point> centers;
cv::Point lastPoint;
cv::Point mousePoint;



void detectEyes(cv::Mat &frame, cv::CascadeClassifier &faceCascade, cv::CascadeClassifier &eyeCascade)
{
	cv::Mat grayscale;
	cv::cvtColor(frame, grayscale, CV_BGR2GRAY); // convert image to grayscale
	cv::equalizeHist(grayscale, grayscale); // enhance image contrast 
	std::vector<cv::Rect> faces;
	faceCascade.detectMultiScale(grayscale, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(150, 150));
	if (faces.size() == 0) return; // none face was detected
	cv::Mat face = grayscale(faces[0]); // crop the face
	std::vector<cv::Rect> eyes;
	eyeCascade.detectMultiScale(face, eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(30, 30)); // same thing as above    
																								//rectangle(frame, faces[0].tl(), faces[0].br(), cv::Scalar(255, 0, 0), 2);
	if (eyes.size() != 2) {
		POINT point;
		iris = false;
		return;
	}
	else {
		iris = true;
	}
	for (cv::Rect &eye : eyes)
	{
		rectangle(frame, faces[0].tl() + eye.tl(), faces[0].tl() + eye.br(), cv::Scalar(0, 255, 0), 2);
	}
	cv::Rect eyeRectLeft = getLeftmostEye(eyes);
	leftEyeTL = eyeRectLeft.tl();
	cv::Mat eyeLeft = face(eyeRectLeft); // crop the leftmost eye
	cv::equalizeHist(eyeLeft, eyeLeft);
	std::vector<cv::Vec3f> circlesL;

	cv::Rect eyeRectRight = getRightmostEye(eyes);
	rightEyeTL = eyeRectRight.tl();
	cv::Mat eyeRight = face(eyeRectRight); // crop the rightmost eye
	cv::equalizeHist(eyeRight, eyeRight);
	std::vector<cv::Vec3f> circlesR;

	cv::HoughCircles(eyeLeft, circlesL, CV_HOUGH_GRADIENT, 1, eyeLeft.cols / 8, 350, 15, eyeLeft.rows / 8, eyeLeft.rows / 3);
	cv::HoughCircles(eyeRight, circlesR, CV_HOUGH_GRADIENT, 1, eyeRight.cols / 8, 350, 15, eyeRight.rows / 8, eyeRight.rows / 3);
	//left iris
	if (circlesL.size() > 0)
	{
		cv::Vec3f eyeball = getEyeball(eyeLeft, circlesL);
		cv::Point center(eyeball[0], eyeball[1]);
		int radius = (int)eyeball[2];
		cv::circle(frame, faces[0].tl() + eyeRectLeft.tl() + center, radius, cv::Scalar(0, 0, 255), 2);
		centerIrisLeft = center + faces[0].tl() + eyeRectLeft.tl();
		centerEyeFinalL.x = centerIrisLeft.x;
		centerEyeFinalL.y = centerIrisLeft.y;
		cv::circle(eyeLeft, center, radius, cv::Scalar(255, 255, 255), 2);
	}
	//for right iris
	if (circlesR.size() > 0)
	{
		cv::Vec3f eyeballR = getEyeball(eyeRight, circlesR);
		cv::Point centerR(eyeballR[0], eyeballR[1]);
		int radius = (int)eyeballR[2];
		cv::circle(frame, faces[0].tl() + eyeRectRight.tl() + centerR, radius, cv::Scalar(0, 0, 255), 2);
		centerIrisRight = faces[0].tl() + eyeRectRight.tl() + centerR;
		centerEyeFinalR.x = centerIrisRight.x;
		centerEyeFinalR.y = centerIrisRight.y;
		cv::circle(eyeRight, centerR, radius, cv::Scalar(255, 255, 255), 2);
	}

}

void Calibrate()
{
	std::cout << "Look in the middle of the screen and click k, please remain still" << std::endl;
	centerIrisCalibR = centerIrisRight;
	centerIrisCalibL = centerIrisLeft;
	leftEyeCalibTL = leftEyeTL;
	rightEyeCalibTL = rightEyeTL;


	rightEyeCalibFinal.x = centerIrisCalibR.x;
	rightEyeCalibFinal.y = centerIrisCalibR.y;

	leftEyeCalibFinal.x = centerIrisCalibL.x;
	leftEyeCalibFinal.y = centerIrisCalibL.y;

	calibration = true;
}

void moveCursor(double x, double y) {
	SetCursorPos(x, y);
}

double getCenterDiffX(cv::Point &centerEyeFinalR, cv::Point &centerEyeFinalL, cv::Point &rightEyeCalibFinal, cv::Point &leftEyeCalibFinal) {
	int xdiffR = centerEyeFinalR.x - rightEyeCalibFinal.x;
	int xdiffL = centerEyeFinalL.x - leftEyeCalibFinal.x;

	double avgxdiff = (xdiffR + xdiffL) / 2;

	return (avgxdiff * -1);
}


double getCenterDiffY(cv::Point &centerEyeFinalR, cv::Point &centerEyeFinalL, cv::Point &rightEyeCalibFinal, cv::Point &leftEyeCalibFinal) {
	int ydiffR = centerEyeFinalR.y - rightEyeCalibFinal.y;
	int ydiffL = centerEyeFinalL.y - leftEyeCalibFinal.y;

	double avgydiff = (ydiffR + ydiffL) / 2;

	return (avgydiff);
}


int main()
{
	cv::CascadeClassifier faceCascade;
	cv::CascadeClassifier eyeCascade;
	faceCascade.load("haarcascade_frontalface_alt2.xml");
	eyeCascade.load("haarcascade_eye.xml");
	cv::VideoCapture cap(0); // the fist webcam connected to your PC
	cv::Mat frame;
	int prevavgxdiff = 0;
	int prevavgydiff = 0;
	int test = 0;
	while (true)
	{
		cap >> frame; // outputs the webcam image to a Mat
					  //if (!frame.data) break;
		detectEyes(frame, faceCascade, eyeCascade);
		if ((iris == false || firstClick == true) && calibration == true) {
			counter++;
			std::cout << counter << std::endl;
			if (counter >= 15) {
				if (firstClick == false) {
					PlaySound(TEXT("mouseclick.wav"), NULL, SND_FILENAME);
					firstClick = true;
				}
				if (iris && counter <= 25) {
					POINT point;
					if (GetCursorPos(&point)) {
						std::cout << "Current X" << point.x << std::endl;
						std::cout << "Current Y" << point.y << std::endl;
					}
					singleLMB(point);
					counter = 0;
					firstClick = false;
				}

			}
		}
		else {
			counter = 0;
		}
		if (test != 0) {
			int avgxdiff = getCenterDiffX(centerEyeFinalR, centerEyeFinalL, rightEyeCalibFinal, leftEyeCalibFinal);
			int avgydiff = getCenterDiffY(centerEyeFinalR, centerEyeFinalL, rightEyeCalibFinal, leftEyeCalibFinal);
			if (abs(avgxdiff) <= 10) {

			}
			else {
				POINT point;
				if (GetCursorPos(&point)) {
				}
				moveCursor(avgxdiff + point.x, avgydiff + point.y);

			}

			if (abs(avgydiff) <= 10) {

			}
			else {
				POINT point;
				if (GetCursorPos(&point)) {
				}
				moveCursor(avgxdiff + point.x, avgydiff + point.y);
			}
			cv::circle(frame, rightEyeCalibFinal, 10, cv::Scalar(255, 255, 255), 2);
			cv::circle(frame, leftEyeCalibFinal, 10, cv::Scalar(255, 255, 255), 2);
		}


		cv::imshow("Webcam", frame); // displays the Mat
		if (cv::waitKey(30) == 'k') {
			std::cout << "Calibrated" << std::endl;
			Calibrate();
			SetCursorPos(500, 500);
			test++;
		}
		else if (cv::waitKey(30) == 'w') {
			break;
		}


	}
	return 0;
}