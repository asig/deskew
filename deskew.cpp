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
const double OUTLIER_PERCENTAGE = 0.05;

const cv::Scalar RED = cv::Scalar(0, 0, 255);
const cv::Scalar GREEN = cv::Scalar(0, 255, 0);
const cv::Scalar BLUE = cv::Scalar(255, 0, 0);
const cv::Scalar WHITE = cv::Scalar(255, 255, 255);

using std::vector;

bool show_preview = false;

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

bool angle_less_than(const cv::Vec4i& l1, const cv::Vec4i& l2) { 
  return angle(l1) < angle(l2);
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

void draw_line(cv::Mat& img, const cv::Vec4i& line, const cv::Scalar& col ) {
    cv::line(img, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), col);
}

void visualize_lines(cv::Mat& img, const vector<cv::Vec4i>& good_lines, const vector<cv::Vec4i>& bad_lines, int skipped_good_lines) {
  for (int i = 0; i < skipped_good_lines; i++) {
    draw_line(img, good_lines[i], BLUE);
  }
  for (int i = skipped_good_lines; i < good_lines.size() - skipped_good_lines; i++) {
    draw_line(img, good_lines[i], GREEN);
  }
  for (int i = good_lines.size() - skipped_good_lines; i < good_lines.size(); i++) {
    draw_line(img, good_lines[i], BLUE);
  }
  for (auto& line : bad_lines) {
    draw_line(img, line, RED);
  }
  if (show_preview) {
    show_image("Lines", img);
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

  if (show_preview) {
    show_image("Resized and b/w", resized);
  }

  // Compute lines     
  std::vector<cv::Vec4i> lines; 
  cv::HoughLinesP(resized, lines, 1, CV_PI/180, 100, size.width/50.f, 20); 

  // Filter and show lines
  std::vector<cv::Vec4i> good_lines, bad_lines; 
  filter_lines(lines, good_lines, bad_lines);

  // sort good lines by angle, and skip the outlying 2% (on each side)
  std::sort(good_lines.begin(), good_lines.end(), angle_less_than);
  int to_skip = good_lines.size() * OUTLIER_PERCENTAGE;
  double total_angle = 0.0;
  for (int i = to_skip; i < good_lines.size() - to_skip; i++) {
    total_angle += angle(good_lines[i]);
  }
  total_angle /= good_lines.size() - 2*to_skip;

  cv::Mat disp_lines(resized.size(), CV_8UC3, cv::Scalar(0, 0, 0));
  visualize_lines(disp_lines, good_lines, bad_lines, to_skip);

  std::cout << lines.size() << " lines in total, " << bad_lines.size() + 2*to_skip << " lines ignored (" << bad_lines.size() << " bad lines)." << std::endl;

  double good_line_ratio = good_lines.size()/((double)lines.size());
  if (good_line_ratio < 0.75) {
    std::cout << "Good lines to lines ratio is " << good_line_ratio << ", refusing to compute a skew angle." << std::endl;
    total_angle = 0;
  }

  return total_angle;
}

void usage() {
  std::cout << "usage: deskew [-preview] <origfile> <destfile> [<origfile> <destfile> ...]" << std::endl;
}

int main(int argc, char** argv) {
  vector<std::string> files;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-preview") {
      show_preview = true;
    } else {
      files.push_back(argv[i]);
    }
  }
  if (files.size() < 2 || files.size() % 2 != 0) {
    usage();
    return 0;
  }
  for (int i = 0; i < files.size(); i+=2) {
    std::string srcfile = files[i];
    std::string destfile = files[i+1];

    cv::Mat orig = cv::imread(srcfile, CV_LOAD_IMAGE_UNCHANGED);
    float skew = compute_skew(orig);
    std::cout << "Skew is " << skew * 180 / CV_PI << " degrees" << std::endl;
 
    cv::Mat dst;
    cv::Point2f pc(orig.cols/2., orig.rows/2.);
    cv::Mat r = cv::getRotationMatrix2D(pc, skew * 180 / CV_PI, 1.0);
    cv::warpAffine(orig, dst, r, orig.size(), cv::INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    cv::imwrite(destfile, dst);
    if (show_preview) {
      show_image("Corrected", dst);
    }
  }
}