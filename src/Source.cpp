#include <opencv2/opencv.hpp>

#include "FlashDetector.hpp"

using namespace std;

using namespace cv;

int main(int argc, char *argv[]) {

	// VideoCapture cap;

	// if (argc > 1) 

	// 	cap.open(argv[1]);

	// else

	// 	cap.open(0);

	// if (!cap.isOpened()) {
	// 	std::cerr << "Cannot initialize video." << std::endl;
	// 	return -1;
	// }

	FlashDetector det;

	//  vector< Vec3b >  palette;

	// for (int i = 0; i<65536; i++)

	// 	palette.push_back(Vec3b((uchar)rand(), (uchar)rand(), (uchar)rand()));

	int num = 0;

	int key = 0;

	vector<KeyPoint>  keyImg;

	vector<int> num_flash;

	vector<int> last_flash;

	Mat frame;



	while (key != 'q' && num < 130) {

		cap.read(frame);



		if (frame.empty()) {

			cerr << "ERROR! blank frame grabbed\n";

			break;

		}



		num++;

		//cout << "frame: " << num << "\n";



		Mat desc, result(frame.rows, frame.cols, CV_8UC3);

		det.detectFlash(frame, keyImg, num_flash, last_flash, num);

		drawKeypoints(frame, keyImg, result);



		int i = 0;



		for (vector<KeyPoint>::iterator k = keyImg.begin(); k != keyImg.end(); k++, i++) 

			circle(result, k->pt, (int)k->size, palette[i % 65536]);



		//int kk = 0;

		//while (kk != 'a') {

		//	imshow("res", result);

		//	kk = cvWaitKey(33);

		//}



		

		key = cvWaitKey(33);

	}



	vector<int>::iterator f = find_if(num_flash.begin(), num_flash.end(), IsFlash);

	if (f != num_flash.end()) {

		KeyPoint fin = keyImg[f - num_flash.begin()];

		cout << fin.pt.x << " " << fin.pt.y << ";   ";

		circle(frame, Point(fin.pt.x, fin.pt.y), 10, 255);

		while (key != 'q') {

			imshow("found you", frame);

			key = cvWaitKey(33);

		}

	}

	else {

		cout << "Nothing found.\n";

	}





	return 0;

}





