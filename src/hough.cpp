#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

const double HOUGH_RHO = 1;
const double HOUGH_THETA = CV_PI / 180;
const int    HOUGH_THRESHOLD = 25;
const double HOUGH_MIN_LEN = 15;
const double HOUGH_MAX_GAP = 50;

vector<Vec4i> detectLines(const Mat& roiEdges)
{
    vector<Vec4i> lines;
    HoughLinesP(roiEdges, lines, HOUGH_RHO, HOUGH_THETA,
        HOUGH_THRESHOLD, HOUGH_MIN_LEN, HOUGH_MAX_GAP);
    return lines;
}

Mat drawAllLines(const Mat& frame, const vector<Vec4i>& lines)
{
    Mat out = frame.clone();
    for (const Vec4i& l : lines)
        line(out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 2, LINE_AA);
    return out;
}