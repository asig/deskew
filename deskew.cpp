#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;

void compute_skew(const char* filename)
{
  // Load in grayscale.
  cv::Mat src = cv::imread(filename, 0);  

  // resize image to something reasonable
  cv::Size size = src.size();
  cv::Size new_size( ((float)size.width)/size.height*600, 600);
  cv::Mat resized;
  cv::resize(src, resized, new_size);

  cv::bitwise_not(resized, resized);

  cv::imshow(filename, resized);
  cv::waitKey(0);
     
  std::vector<cv::Vec4i> lines; 
  cv::HoughLinesP(resized, lines, 1, CV_PI/180, 100, size.width / 50.f, 20);
  // cv::HoughLinesP(src, lines, 1, CV_PI/180, 100, 40, 20);
  cv::Mat disp_lines(new_size, CV_8UC1, cv::Scalar(0, 0, 0));

  double total_angle = 0.;
  unsigned nb_lines = lines.size();
  unsigned int lines_used = 0;
  for (unsigned i = 0; i < nb_lines; ++i) {
        double angle = atan2((double)lines[i][3] - lines[i][1],
                    (double)lines[i][2] - lines[i][0]);
        double angle_deg = angle * 180 / CV_PI;
        if (angle_deg > -20 && angle_deg < 20) {
          cv::line(disp_lines, cv::Point(lines[i][0], lines[i][1]), cv::Point(lines[i][2], lines[i][3]), cv::Scalar(255, 0 ,0));

          total_angle += angle;
          lines_used++;
        }
    }
    total_angle /= lines_used; // mean angle, in radians.

    cout << nb_lines << " lines in total, " << (nb_lines - lines_used) << " lines ignored." << endl;
 
    std::cout << "File " << filename << ": " << total_angle * 180 / CV_PI << std::endl;
 
    cv::imshow(filename, disp_lines);
    cv::waitKey(0);
    cv::destroyWindow(filename);
}

void usage() {
  cout << "usage: deskew <origfile> <destfile> [<origfile> <destfile> ...]" << endl;
}

int main(int argc, char** argv) {
  if (argc < 2 || argc % 2 == 0) {
    usage();
    return 0;
  }
  for (int i = 1; i < argc; i+=2) {
    char *srcfile = argv[i];
    char *destfile = argv[i+1];
    compute_skew(srcfile);
  }
}