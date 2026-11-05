#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

const double SLOPE_MIN = 0.5;
const double SLOPE_MAX = 5.0;

// 이미지는 y가 아래로 커지므로 왼쪽 차선은 기울기 음수, 오른쪽 차선은 양수
void separateLines(const vector<Vec4i>& lines, int width,
    vector<Vec4i>& leftLines, vector<Vec4i>& rightLines)
{
    leftLines.clear();
    rightLines.clear();
    double centerX = width / 2.0;

    for (const Vec4i& l : lines) {
        int x1 = l[0], y1 = l[1], x2 = l[2], y2 = l[3];
        if (x2 == x1) continue;

        double slope = (double)(y2 - y1) / (x2 - x1);
        double absS = fabs(slope);
        if (absS < SLOPE_MIN || absS > SLOPE_MAX) continue;

        double midX = (x1 + x2) / 2.0;

        if (slope < 0 && midX < centerX)       leftLines.push_back(l);
        else if (slope > 0 && midX > centerX)  rightLines.push_back(l);
    }
}

Mat drawFilteredLines(const Mat& frame,
    const vector<Vec4i>& leftLines,
    const vector<Vec4i>& rightLines)
{
    Mat out = frame.clone();
    for (const Vec4i& l : leftLines)
        line(out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255, 0, 0), 3, LINE_AA);
    for (const Vec4i& l : rightLines)
        line(out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 0, 255), 3, LINE_AA);
    return out;
}