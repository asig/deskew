/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2016 Andreas Signer <asigner@gmail.com>
*/

#include <iostream>
#include <opencv2/opencv.hpp>

const int RESIZED_HEIGHT = 1200;
const int BW_THRESHOLD = 60;

const cv::Scalar RED = cv::Scalar(0, 0, 255);
const cv::Scalar GREEN = cv::Scalar(0, 255, 0);
const cv::Scalar WHITE = cv::Scalar(255, 255, 255);

using std::vector;

void show_image(const std::string& name, cv::Mat& img) {
  cv::imshow(name, img);  
  cv::waitKey(0);
  cv::destroyWindow(name);
}

float angle(const cv::Vec4i& line) {
  double dx = line[2] - line[0];
  double dy = line[3] - line[1];
  return atan2(dy, dx);
}

float deg(float angle) {  
  return angle * 180 / CV_PI;
}

void filter_lines(const vector<cv::Vec4i>& in, vector<cv::Vec4i>& good_lines, vector<cv::Vec4i>& bad_lines) {
  for (unsigned i = 0; i < in.size(); ++i) {
        double a = angle(in[i]);
        if (deg(a) > -15 && deg(a) < 15) {
          good_lines.push_back(in[i]);
        } else {
          bad_lines.push_back(in[i]);
        }
    }
}

float compute_skew(const cv::Mat& img) {
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

  show_image("Resized and b/w", resized);

  // Compute lines     
  std::vector<cv::Vec4i> lines; 
  cv::HoughLinesP(resized, lines, 1, CV_PI/180, 200, size.width/50.f, 20); 

  // Filter and show lines
  cv::Mat disp_lines(resized.size(), CV_8UC3, cv::Scalar(0, 0, 0));
  std::vector<cv::Vec4i> good_lines, bad_lines; 
  filter_lines(lines, good_lines, bad_lines);

  double total_angle = 0.;
  for (auto& line : good_lines) {
    total_angle += angle(line);
    cv::line(disp_lines, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), GREEN);
  }
  total_angle /= good_lines.size();  
  for (auto& line : bad_lines) {
    cv::line(disp_lines, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), RED);
  }
  show_image("Lines", disp_lines);

  std::cout << lines.size() << " lines in total, " << bad_lines.size() << " lines ignored." << std::endl;

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
    show_image("Corrected", dst);
  }
}