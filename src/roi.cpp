#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// ROI 파라미터. 모든 값은 프레임 너비(W)/높이(H) 대비 비율(0~1)
struct ROIParam {
    double botLeftX, botRightX, botY;
    double topLeftX, topRightX, topY;
};

// 영상별 ROI (topY는 main.cpp의 TOP_RATIO, overlay.cpp의 topRatio와 같아야 함)
static const ROIParam PROFILES[5] = {
    // botL  botR  botY  topL  topR  topY
    { 0.02, 0.95, 1.00, 0.40, 0.48, 0.60 },
    { 0.02, 0.95, 1.00, 0.40, 0.50, 0.60 },
    { 0.02, 0.95, 1.00, 0.40, 0.50, 0.60 },
    { 0.13, 0.70, 0.75, 0.36, 0.52, 0.45 },
    { 0.25, 0.80, 0.80, 0.45, 0.52, 0.52 }
};

static ROIParam g_roi = PROFILES[0];

void setROIProfile(int idx)
{
    if (idx < 0 || idx > 4) idx = 0;
    g_roi = PROFILES[idx];
}

static vector<Point> roiPolygon(Size sz)
{
    int W = sz.width, H = sz.height;
    int yBot = min(H - 1, (int)(H * g_roi.botY));
    return {
        Point((int)(W * g_roi.botLeftX),  yBot),
        Point((int)(W * g_roi.botRightX), yBot),
        Point((int)(W * g_roi.topRightX), (int)(H * g_roi.topY)),
        Point((int)(W * g_roi.topLeftX),  (int)(H * g_roi.topY))
    };
}

Mat applyROI(const Mat& edges)
{
    Mat roi = Mat::zeros(edges.size(), CV_8UC1);
    vector<vector<Point>> polys = { roiPolygon(edges.size()) };
    fillPoly(roi, polys, Scalar(255));
    Mat result;
    bitwise_and(edges, roi, result);
    return result;
}

void drawROI(Mat& frame)
{
    vector<Point> poly = roiPolygon(frame.size());
    polylines(frame, poly, true, Scalar(0, 255, 0), 2);
}