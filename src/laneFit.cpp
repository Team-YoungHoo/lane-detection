#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

bool fitLane(const vector<Vec4i>& lines, int H, double topRatio,
    double& xBottom, double& xTop)
{
    if (lines.empty()) return false;

    vector<Point2f> pts;
    for (const Vec4i& l : lines) {
        pts.push_back(Point2f((float)l[0], (float)l[1]));
        pts.push_back(Point2f((float)l[2], (float)l[3]));
    }

    Vec4f f;
    fitLine(pts, f, DIST_L2, 0, 0.01, 0.01);
    float vx = f[0], vy = f[1], x0 = f[2], y0 = f[3];
    if (fabs(vy) < 1e-6f) return false;   // 수평선이면 x=f(y)를 못 구하므로 실패

    xBottom = x0 + (H - y0) * vx / vy;
    xTop = x0 + (topRatio * H - y0) * vx / vy;
    return true;
}