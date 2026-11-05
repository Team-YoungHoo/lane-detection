#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
using namespace cv;
using namespace std;

const double OVERLAY_ALPHA = 0.3;
const int LINE_THICK = 6;
static int displayProfile = 0;
void setLaneDisplayProfile(int idx) { displayProfile = (idx >= 0 && idx < 5) ? idx : 0; }

// xb는 y=H 기준 값이므로 표시 하단 y로 옮기지 않고 다시 계산한다.
static double laneX(double xb, double xt, double topRatio, int H, double y)
{
    return xb + (xt - xb) * (H - y) / (H - topRatio * H);
}

static bool displayPoints(const Mat& frame, bool lv, double lb, double lt,
    bool rv, double rb, double rt, double tr,
    Point& pLB, Point& pLT, Point& pRB, Point& pRT)
{
    if (frame.empty() || tr < 0 || tr >= 1 ||
        (lv && (!isfinite(lb) || !isfinite(lt))) ||
        (rv && (!isfinite(rb) || !isfinite(rt)))) return false;
    const int H = frame.rows;
    const double bottomRatio[5] = { 1.0, 1.0, 1.0, 0.75, 0.80 };
    const double topRatio[5] = { 0.60, 0.60, 0.60, 0.45, 0.52 };
    int yB = min(H - 1, (int)(H * bottomRatio[displayProfile]));
    int yT = (int)ceil(H * max(tr, topRatio[displayProfile]));
    if (yT >= yB) return false;

    if (lv && rv && displayProfile >= 3) {
        const double minGap = 4.0;
        auto gap = [&](int y) {
            return laneX(rb, rt, tr, H, y) - laneX(lb, lt, tr, H, y);
            };
        if (gap(yB) <= minGap) return false;
        while (yT < yB && gap(yT) <= minGap) ++yT;
        if (yB - yT < 5) return false;
    }
    if (lv) {
        pLB = Point(cvRound(laneX(lb, lt, tr, H, yB)), yB);
        pLT = Point(cvRound(laneX(lb, lt, tr, H, yT)), yT);
    }
    if (rv) {
        pRB = Point(cvRound(laneX(rb, rt, tr, H, yB)), yB);
        pRT = Point(cvRound(laneX(rb, rt, tr, H, yT)), yT);
    }
    return true;
}

Mat drawLaneLinesOnly(const Mat& frame, bool lv, double lxB, double lxT,
    bool rv, double rxB, double rxT, double topRatio)
{
    Mat out = frame.clone();
    Point lb, lt, rb, rt;
    if (!displayPoints(frame, lv, lxB, lxT, rv, rxB, rxT, topRatio,
        lb, lt, rb, rt)) return out;
    if (lv) line(out, lb, lt, Scalar(255, 0, 0), LINE_THICK, LINE_AA);
    if (rv) line(out, rb, rt, Scalar(0, 0, 255), LINE_THICK, LINE_AA);
    return out;
}

Mat drawResult(const Mat& frame, double lxB, double lxT,
    double rxB, double rxT, double topRatio, const Scalar& fillColor)
{
    Point lb, lt, rb, rt;
    if (!displayPoints(frame, true, lxB, lxT, true, rxB, rxT, topRatio,
        lb, lt, rb, rt)) return frame.clone();
    Mat overlay = frame.clone(), out;
    vector<vector<Point>> poly = { { lb, lt, rt, rb } };
    fillPoly(overlay, poly, fillColor);
    addWeighted(overlay, OVERLAY_ALPHA, frame, 1.0 - OVERLAY_ALPHA, 0, out);
    line(out, lb, lt, Scalar(255, 0, 0), LINE_THICK, LINE_AA);
    line(out, rb, rt, Scalar(0, 0, 255), LINE_THICK, LINE_AA);
    return out;
}
