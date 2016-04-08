#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace std;

void compute_skew(const char* filename)
{
  // Load in grayscale.
  cv::Mat src = cv::imread(filename, 0);
  cout << "File loaded" << endl;

  cv::Size size = src.size();
  cv::bitwise_not(src, src);
 cv::imshow(filename, src);
    cv::waitKey(0);
     
  std::vector<cv::Vec4i> lines; 
  cv::HoughLinesP(src, lines, 1, CV_PI/180, 100, size.width / 50.f, 20);
  // cv::HoughLinesP(src, lines, 1, CV_PI/180, 100, 40, 20);
  cv::Mat disp_lines(size, CV_8UC1, cv::Scalar(0, 0, 0));

  double angle = 0.;
  unsigned nb_lines = lines.size();
  for (unsigned i = 0; i < nb_lines; ++i) {
        cv::line(disp_lines, cv::Point(lines[i][0], lines[i][1]),
                 cv::Point(lines[i][2], lines[i][3]), cv::Scalar(255, 0 ,0));
        angle += atan2((double)lines[i][3] - lines[i][1],
                       (double)lines[i][2] - lines[i][0]);
    }
    angle /= nb_lines; // mean angle, in radians.
 
    std::cout << "File " << filename << ": " << angle * 180 / CV_PI << std::endl;
 
    cv::imshow(filename, disp_lines);
    cv::waitKey(0);
    cv::destroyWindow(filename);
}


int main(int argc, char** argv) {
	compute_skew(argv[1]);
}