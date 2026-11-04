// =====================================================================
// hough.cpp  -  3단계 ④ 허프 변환
// ---------------------------------------------------------------------
// 역할 : ROI가 적용된 에지 영상에서 직선 후보(선분)를 찾는다.
// 입력 : ROI 에지 영상 (1채널)
// 출력 : 선분 목록 vector<Vec4i>, 각 원소는 (x1, y1, x2, y2)
// 호출 : main.cpp  (detectLines, drawAllLines)
// 튜닝 : 선이 너무 많이 잡히면 HOUGH_THRESHOLD 를 올리고,
//        점선 차선이 끊겨 안 잡히면 HOUGH_MAX_GAP 을 올린다.
// =====================================================================
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// ===== HoughLinesP 파라미터 (과제 사양값) =====
const double HOUGH_RHO = 1;                // 거리 해상도(픽셀)
const double HOUGH_THETA = CV_PI / 180;    // 각도 해상도(1도, 라디안)
const int    HOUGH_THRESHOLD = 30;         // 직선으로 인정할 최소 투표 수. 선이 너무 많으면 ↑
const double HOUGH_MIN_LEN = 20;           // 이보다 짧은 선분은 버림
const double HOUGH_MAX_GAP = 50;           // 같은 선으로 이어 줄 최대 끊김 간격. 점선이 안 잡히면 ↑

// [3-④] 확률적 허프 변환(HoughLinesP)으로 직선 후보를 검출
vector<Vec4i> detectLines(const Mat& roiEdges)
{
    vector<Vec4i> lines;
    HoughLinesP(roiEdges, lines, HOUGH_RHO, HOUGH_THETA,
        HOUGH_THRESHOLD, HOUGH_MIN_LEN, HOUGH_MAX_GAP);
    return lines;
}

// 확인용: 후보 선을 모두 같은 색(초록)으로 원본 위에 그려서 반환
// (원본 frame은 건드리지 않고 복사본에 그림)
Mat drawAllLines(const Mat& frame, const vector<Vec4i>& lines)
{
    Mat out = frame.clone();
    for (const Vec4i& l : lines)
        line(out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 2, LINE_AA);
    return out;
}