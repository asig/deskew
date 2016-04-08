#include <iostream>
#include <opencv2/opencv.hpp>

const int RESIZED_HEIGHT = 1200;
const int BW_THRESHOLD = 60;

void showImage(const std::string& name, cv::Mat& img) {
  cv::imshow(name, img);  
  cv::waitKey(0);
  cv::destroyWindow(name);
}

float compute_skew(const cv::Mat& img)
{
  // resize image to something reasonable
  cv::Size size = img.size();
  cv::Mat resized;
  if (size.height > RESIZED_HEIGHT) {
    cv::Size new_size( ((float)size.width)/size.height*RESIZED_HEIGHT, RESIZED_HEIGHT);
    cv::resize(img, resized, new_size);
  } else {
    resized = img;
  }

  // Convert it to white on black
  if (resized.channels() > 1) {
    cv::cvtColor(resized, resized, CV_BGR2GRAY);
  }
  cv::bitwise_not(resized, resized);
  resized = resized > BW_THRESHOLD;

  showImage("Resized and b/w", resized);

  // Compute lines     
  std::vector<cv::Vec4i> lines; 
  cv::HoughLinesP(resized, lines, 1, CV_PI/180, 100, size.width/50.f, 20); 
  
  cv::Mat disp_lines(resized.size(), CV_8UC3, cv::Scalar(0, 0, 0));

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

    std::cout << nb_lines << " lines in total, " << (nb_lines - lines_used) << " lines ignored." << std::endl;
 
    showImage("Lines", disp_lines);

    return total_angle;
}

void usage() {
  std::cout << "usage: deskew <origfile> <destfile> [<origfile> <destfile> ...]" << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2 || argc % 2 == 0) {
    usage();
    return 0;
  }
  for (int i = 1; i < argc; i+=2) {
    char *srcfile = argv[i];
    char *destfile = argv[i+1];

    cv::Mat orig = cv::imread(srcfile, CV_LOAD_IMAGE_UNCHANGED);
    float skew = compute_skew(orig);
    std::cout << "Skew is " << skew * 180 / CV_PI << " degrees" << std::endl;
 
    cv::Mat dst;
    cv::Point2f pc(orig.cols/2., orig.rows/2.);
    cv::Mat r = cv::getRotationMatrix2D(pc, skew * 180 / CV_PI, 1.0);
    cv::warpAffine(orig, dst, r, orig.size(), cv::INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    cv::imwrite(destfile, dst);
    showImage("Corrected", dst);
  }
}