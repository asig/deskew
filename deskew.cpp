#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;

const int RESIZED_HEIGHT = 1200;
const int BW_THRESHOLD = 60;

void showImage(const std::string& name, cv::Mat& img) {
  cv::imshow(name, img);  
  cv::waitKey(0);
  cv::destroyWindow(name);
}

void compute_skew(const char* filename)
{
  // Load in grayscale.
  cv::Mat src = cv::imread(filename, 0);

  // resize image to something reasonable
  cv::Size size = src.size();
  cv::Size new_size( ((float)size.width)/size.height*RESIZED_HEIGHT, RESIZED_HEIGHT);
  cv::Mat resized;
  cv::resize(src, resized, new_size);

  // Convert it to white on black
  cv::bitwise_not(resized, resized);
  resized = resized > BW_THRESHOLD;

  showImage("Resized and b/w", resized);

  // Compute lines     
  std::vector<cv::Vec4i> lines; 
  cv::HoughLinesP(resized, lines, 1, CV_PI/180, 100, size.width/50.f, 20); 
  
  cv::Mat disp_lines(new_size, CV_8UC3, cv::Scalar(0, 0, 0));

  // Filter and show lines
  double total_angle = 0.;
  unsigned nb_lines = lines.size();
  unsigned int lines_used = 0;
  const cv::Scalar RED = cv::Scalar(0, 0, 255);
  const cv::Scalar GREEN = cv::Scalar(0, 255, 0);
  for (unsigned i = 0; i < nb_lines; ++i) {
        double angle = atan2((double)lines[i][3] - lines[i][1],
                    (double)lines[i][2] - lines[i][0]);
        double angle_deg = angle * 180 / CV_PI;
        if (angle_deg > -15 && angle_deg < 15) {
          cv::line(disp_lines, cv::Point(lines[i][0], lines[i][1]), cv::Point(lines[i][2], lines[i][3]), GREEN);
          total_angle += angle;
          lines_used++;
        } else {
          cv::line(disp_lines, cv::Point(lines[i][0], lines[i][1]), cv::Point(lines[i][2], lines[i][3]), RED);
        }

    }
    total_angle /= lines_used; // mean angle, in radians.

    cout << nb_lines << " lines in total, " << (nb_lines - lines_used) << " lines ignored." << endl;
 
    std::cout << "File " << filename << ": " << total_angle * 180 / CV_PI << std::endl;
 
    showImage("Lines", disp_lines);
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