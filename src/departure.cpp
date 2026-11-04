// =====================================================================
// departure.cpp : 9. 오프셋 / 차선 이탈 경고 (과제 4단계)
// ---------------------------------------------------------------------
// 과제의 식을 그대로 사용:
//   차량 중심 = W / 2.0   (카메라가 차량 정중앙에 있다고 가정)
//   차로 중심 = (평활화된 왼쪽 하단 x + 오른쪽 하단 x) / 2
//   차로 폭   = 오른쪽 하단 x - 왼쪽 하단 x
//   오프셋    = (차량 중심 - 차로 중심) / 차로 폭
//   offset > 0 : 차량이 차로 중심보다 오른쪽
//   offset < 0 : 차량이 차로 중심보다 왼쪽
//
// 기존 main.cpp와 함수/구조체가 같으므로 이 파일만 교체 가능.
// 입력은 fitLane/smoothLane의 y=H 기준 좌표. 선 표시를 보닛 위에서
// 자르더라도, 입력 좌표의 기준 높이는 임의로 바꾸지 않는다.
//
// 주의: 영상 좌표 기반 과제용 추정이다. 실제 거리/바퀴 접촉을 확정하지
// 못하며 방향지시등 정보가 없어 의도적인 차선 변경도 경고할 수 있다.
// =====================================================================
#include <opencv2/opencv.hpp>
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace cv;
using namespace std;

// 안내문 예시: (3.5 - 1.8)/2 / 3.5 = 0.242857...
// 0.24는 예시 기반 초기값이지 5개 영상에서 실측 보정된 값은 아니다.
const double DEPART_TH = 0.24;
const double WARN_TH = 0.12; // 선택 기능: 이탈 임계값의 절반부터 주의
const double MIN_WIDTH_PX = 1.0; // 영/음수 차로 폭 및 퇴화한 입력 방지
const double CENTER_TEXT_EPS = 0.005; // 방향 문구에만 사용; 판정식은 변경하지 않음

// main.cpp의 정의와 반드시 동일하게 유지.
struct DepartInfo {
    double carCenter = 0, laneCenter = 0, laneWidth = 0, offset = 0;
    int state = 0;        // 0 정상 / 1 주의 / 2 이탈 경고
    bool valid = false;  // 판단 불가이면 false (정상으로 해석하면 안 됨)
};

DepartInfo checkDeparture(int W, double lXb, double rXb)
{
    DepartInfo d;
    if (W <= 0 || !std::isfinite(lXb) || !std::isfinite(rXb)) return d;

    const double laneWidth = rXb - lXb;
    // 좌우 역전/동일 좌표/극단적인 수치 연산 결과는 판단하지 않는다.
    if (!std::isfinite(laneWidth) || laneWidth < MIN_WIDTH_PX) return d;

    const double carCenter = W / 2.0;
    const double laneCenter = lXb + laneWidth / 2.0;
    const double offset = (carCenter - laneCenter) / laneWidth;
    if (!std::isfinite(laneCenter) || !std::isfinite(offset)) return d;

    d.carCenter = carCenter;
    d.laneCenter = laneCenter;
    d.laneWidth = laneWidth;
    d.offset = offset;
    const double a = std::fabs(offset);
    d.state = a >= DEPART_TH ? 2 : (a >= WARN_TH ? 1 : 0);
    d.valid = true;
    return d;
}

Mat drawDeparture(const Mat& frame, const DepartInfo& d)
{
    if (frame.empty()) return frame.clone();
    Mat out = frame.clone();
    const int W = out.cols, H = out.rows;
    const bool usable = d.valid && d.state >= 0 && d.state <= 2 &&
        std::isfinite(d.carCenter) && std::isfinite(d.laneCenter) &&
        std::isfinite(d.laneWidth) && std::isfinite(d.offset) &&
        d.laneWidth >= MIN_WIDTH_PX;

    // 640x360을 기준으로 화면 크기에 맞춤. OpenCV 기본 폰트로 영문 표시.
    const double s = std::min(W / 640.0, H / 360.0);
    auto point = [&](int x, int y) {
        return Point(cvRound(x * s), cvRound(y * s));
        };
    const int thick = std::max(1, cvRound(2 * s));
    // 밝은 도로에서도 숫자가 읽히도록 정보 패널 배경을 어둡게 합성.
    Mat panel = out.clone();
    rectangle(panel, Rect(0, 0, W, std::min(H, std::max(1, cvRound(139 * s)))),
        Scalar(0, 0, 0), FILLED);
    addWeighted(panel, 0.6, out, 0.4, 0, out);

    if (!usable) {
        putText(out, "LANE UNAVAILABLE - NO JUDGEMENT", point(14, 32),
            FONT_HERSHEY_SIMPLEX, 0.62 * s, Scalar(200, 200, 200), thick, LINE_AA);
        putText(out, "Missing / invalid lane boundaries", point(14, 60),
            FONT_HERSHEY_SIMPLEX, 0.52 * s, Scalar(200, 200, 200), thick, LINE_AA);
        return out;
    }

    const char* direction = std::fabs(d.offset) < CENTER_TEXT_EPS ? "CENTER" :
        (d.offset > 0 ? "RIGHT" : "LEFT");
    const char* messages[3] = { "OK", "CAUTION", "LANE DEPARTURE WARNING" };
    const Scalar colors[3] = {
        Scalar(0, 220, 0), Scalar(0, 220, 255), Scalar(0, 0, 255)
    };
    char text[180];
    std::snprintf(text, sizeof(text), "Offset: %+.3f (%+.1f%% of lane width)",
        d.offset, d.offset * 100.0);
    putText(out, text, point(14, 28), FONT_HERSHEY_SIMPLEX, 0.59 * s,
        Scalar(255, 255, 255), thick, LINE_AA);
    std::snprintf(text, sizeof(text), "Vehicle position: %s", direction);
    putText(out, text, point(14, 54), FONT_HERSHEY_SIMPLEX, 0.57 * s,
        Scalar(255, 255, 255), thick, LINE_AA);
    putText(out, messages[d.state], point(14, 85), FONT_HERSHEY_SIMPLEX,
        0.70 * s, colors[d.state], thick, LINE_AA);
    std::snprintf(text, sizeof(text), "Warning threshold: |offset| >= %.2f", DEPART_TH);
    putText(out, text, point(14, 109), FONT_HERSHEY_SIMPLEX, 0.48 * s,
        Scalar(220, 220, 220), std::max(1, cvRound(s)), LINE_AA);
    putText(out, "Image estimate / centered camera / reference y=H", point(14, 130),
        FONT_HERSHEY_SIMPLEX, 0.43 * s, Scalar(220, 220, 220),
        std::max(1, cvRound(s)), LINE_AA);

    // 입력 기준은 y=H. 화면 안쪽에 짧은 중심 눈금을 표시.
    // ROI 하단이 아니라 화면 하단으로 연장한 중심의 x 위치를 나타냄.
    const int yBottom = H - 1;
    const int yTop = std::max(0, yBottom - cvRound(45 * s));
    auto tick = [&](double x, Scalar color) {
        if (x >= 0 && x <= W - 1)
            line(out, Point(cvRound(x), yBottom), Point(cvRound(x), yTop),
                color, std::max(1, cvRound(3 * s)), LINE_AA);
        };
    tick(d.carCenter, Scalar(0, 255, 255)); // 노랑: 차량 중심
    tick(d.laneCenter, Scalar(0, 255, 0)); // 초록: 차로 중심
    if (d.state == 2)
        rectangle(out, Point(0, 0), Point(W - 1, H - 1),
            Scalar(0, 0, 255), std::max(1, cvRound(6 * s)));
    return out;
}
