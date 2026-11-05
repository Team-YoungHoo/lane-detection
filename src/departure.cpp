#include <opencv2/opencv.hpp>
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace cv;
using namespace std;

// 안내문 예시: (3.5 - 1.8)/2 / 3.5 = 0.242857...
const double DEPART_TH = 0.24;
// 영상별 정상 주행 오프셋의 중앙값 (키보드 1~5번 영상). 영상을 바꾸면 다시 구해야 한다.
const double OFFSET_BIAS[5] = { 0.144, 0.131, 0.069, 0.083, 0.004 };
const int DEPART_MIN_FRAMES = 5;
const int HOLD_MAX_FRAMES = 180;
const double MIN_WIDTH_PX = 1.0;
const double CENTER_TEXT_EPS = 0.005;

// turn_signal.cpp의 SIGNAL_LEFT / SIGNAL_RIGHT 값과 동일하게 유지.
const int SIGNAL_LEFT = 1, SIGNAL_RIGHT = 2;
const int BLINK_FRAMES = 8;

// 아래 enum과 구조체는 main.cpp의 정의와 반드시 동일하게 유지.
enum LaneState { ST_SAFE, ST_WARNING, ST_CHANGE, ST_LOST };
struct DepartInfo {
    double carCenter = 0, laneCenter = 0, laneWidth = 0, offset = 0;
    int state = ST_LOST;
    bool valid = false;
    int direction = 0;
    bool held = false;
};

static double g_offsetBias = OFFSET_BIAS[0];
static int g_departFrames = 0;
static int g_holdDir = 0;
static int g_holdState = ST_WARNING;
static int g_holdFrames = 0;

void setDepartureProfile(int idx)
{
    g_offsetBias = OFFSET_BIAS[(idx >= 0 && idx < 5) ? idx : 0];
    g_departFrames = 0;
    g_holdDir = 0;
    g_holdState = ST_WARNING;
    g_holdFrames = 0;
}

Scalar laneStateColor(int state)
{
    if (state == ST_WARNING) return Scalar(0, 0, 255);
    if (state == ST_CHANGE) return Scalar(0, 255, 255);
    return Scalar(0, 255, 0);
}

static DepartInfo checkDeparture(int W, double lXb, double rXb, int signal)
{
    DepartInfo d;
    if (W <= 0 || !std::isfinite(lXb) || !std::isfinite(rXb)) return d;

    const double laneWidth = rXb - lXb;
    if (!std::isfinite(laneWidth) || laneWidth < MIN_WIDTH_PX) return d;

    const double carCenter = W / 2.0 - g_offsetBias * laneWidth;
    const double laneCenter = lXb + laneWidth / 2.0;
    const double offset = (carCenter - laneCenter) / laneWidth;
    if (!std::isfinite(laneCenter) || !std::isfinite(offset)) return d;

    d.carCenter = carCenter;
    d.laneCenter = laneCenter;
    d.laneWidth = laneWidth;
    d.offset = offset;
    if (std::fabs(offset) < DEPART_TH) d.state = ST_SAFE;
    else {
        // offset > 0 이면 오른쪽으로 이탈
        const int departDir = offset > 0 ? SIGNAL_RIGHT : SIGNAL_LEFT;
        d.state = signal == departDir ? ST_CHANGE : ST_WARNING;
        d.direction = departDir;
    }
    d.valid = true;
    return d;
}

DepartInfo updateDeparture(bool ready, int W, double lXb, double rXb, int signal)
{
    DepartInfo d;
    if (ready) d = checkDeparture(W, lXb, rXb, signal);

    if (d.valid) {
        g_departFrames = d.state == ST_SAFE ? 0 : g_departFrames + 1;
        if (d.state != ST_SAFE && g_departFrames < DEPART_MIN_FRAMES) {
            d.state = ST_SAFE;
            d.direction = 0;
        }
        g_holdDir = d.direction;
        g_holdState = d.state;
        g_holdFrames = 0;
        return d;
    }

    // 차선을 놓침: 직전에 이탈 중이었다면 그 상태를 유지
    g_departFrames = 0;
    if (g_holdDir == 0 || g_holdFrames >= HOLD_MAX_FRAMES) {
        g_holdDir = 0;
        return d;
    }
    ++g_holdFrames;
    // 지시등이 도중에 꺼져도(3초) 진행 중인 차선 변경은 경고로 바꾸지 않는다.
    if (signal == g_holdDir) g_holdState = ST_CHANGE;
    d.state = g_holdState;
    d.direction = g_holdDir;
    d.held = true;
    return d;
}

Mat drawDeparture(const Mat& frame, const DepartInfo& d, int signal, int frameNo)
{
    if (frame.empty()) return frame.clone();
    Mat out = frame.clone();
    const int W = out.cols, H = out.rows;
    const bool usable = d.valid && d.state != ST_LOST &&
        std::isfinite(d.carCenter) && std::isfinite(d.laneCenter) &&
        std::isfinite(d.laneWidth) && std::isfinite(d.offset) &&
        d.laneWidth >= MIN_WIDTH_PX;

    const double s = std::min(W / 640.0, H / 360.0);
    auto point = [&](int x, int y) {
        return Point(cvRound(x * s), cvRound(y * s));
        };
    const int thick = std::max(1, cvRound(2 * s));
    Mat panel = out.clone();
    rectangle(panel, Rect(0, 0, W, std::min(H, std::max(1, cvRound(139 * s)))),
        Scalar(0, 0, 0), FILLED);
    addWeighted(panel, 0.6, out, 0.4, 0, out);

    const char* signalText = signal == SIGNAL_LEFT ? "LEFT" :
        (signal == SIGNAL_RIGHT ? "RIGHT" : "OFF");
    if (signal == SIGNAL_LEFT || signal == SIGNAL_RIGHT) {
        const Point a = point(signal == SIGNAL_LEFT ? 625 : 555, 85);
        const Point b = point(signal == SIGNAL_LEFT ? 555 : 625, 85);
        arrowedLine(out, a, b, Scalar(0, 255, 255), std::max(1, cvRound(5 * s)),
            LINE_AA, 0, 0.4);
    }

    const char* departText = d.direction == SIGNAL_RIGHT ? "RIGHT" : "LEFT";
    char message[80];
    Scalar messageColor;
    if (d.state == ST_WARNING) {
        std::snprintf(message, sizeof(message), "LANE DEPARTURE WARNING (%s)", departText);
        messageColor = Scalar(0, 0, 255);
    }
    else if (d.state == ST_CHANGE) {
        std::snprintf(message, sizeof(message), "Lane change: %s", departText);
        messageColor = Scalar(0, 255, 255);
    }
    else {
        std::snprintf(message, sizeof(message), "Current state: Safe");
        messageColor = Scalar(0, 220, 0);
    }
    auto drawWarningBorder = [&]() {
        if (d.state == ST_WARNING && (frameNo / BLINK_FRAMES) % 2 == 0)
            rectangle(out, Point(0, 0), Point(W - 1, H - 1),
                Scalar(0, 0, 255), std::max(1, cvRound(6 * s)));
        };

    char text[180];
    if (d.held) {
        putText(out, message, point(14, 32), FONT_HERSHEY_SIMPLEX,
            0.70 * s, messageColor, thick, LINE_AA);
        std::snprintf(text, sizeof(text), "Lane lines lost while departing | Signal: %s", signalText);
        putText(out, text, point(14, 60),
            FONT_HERSHEY_SIMPLEX, 0.52 * s, Scalar(200, 200, 200), thick, LINE_AA);
        drawWarningBorder();
        return out;
    }
    if (!usable) {
        putText(out, "Lane not detected", point(14, 32),
            FONT_HERSHEY_SIMPLEX, 0.70 * s, Scalar(200, 200, 200), thick, LINE_AA);
        std::snprintf(text, sizeof(text), "Signal: %s", signalText);
        putText(out, text, point(14, 60),
            FONT_HERSHEY_SIMPLEX, 0.52 * s, Scalar(200, 200, 200), thick, LINE_AA);
        return out;
    }

    const char* direction = std::fabs(d.offset) < CENTER_TEXT_EPS ? "CENTER" :
        (d.offset > 0 ? "RIGHT" : "LEFT");
    std::snprintf(text, sizeof(text), "Offset: %+.3f (%+.1f%% of lane width)",
        d.offset, d.offset * 100.0);
    putText(out, text, point(14, 28), FONT_HERSHEY_SIMPLEX, 0.59 * s,
        Scalar(255, 255, 255), thick, LINE_AA);
    std::snprintf(text, sizeof(text), "Vehicle position: %s | Signal: %s",
        direction, signalText);
    putText(out, text, point(14, 54), FONT_HERSHEY_SIMPLEX, 0.57 * s,
        Scalar(255, 255, 255), thick, LINE_AA);
    putText(out, message, point(14, 85), FONT_HERSHEY_SIMPLEX,
        0.70 * s, messageColor, thick, LINE_AA);
    std::snprintf(text, sizeof(text), "Warning threshold: |offset| >= %.2f", DEPART_TH);
    putText(out, text, point(14, 109), FONT_HERSHEY_SIMPLEX, 0.48 * s,
        Scalar(220, 220, 220), std::max(1, cvRound(s)), LINE_AA);
    putText(out, "Image estimate / calibrated center / reference y=H", point(14, 130),
        FONT_HERSHEY_SIMPLEX, 0.43 * s, Scalar(220, 220, 220),
        std::max(1, cvRound(s)), LINE_AA);

    const int yBottom = H - 1;
    const int yTop = std::max(0, yBottom - cvRound(45 * s));
    auto tick = [&](double x, Scalar color) {
        if (x >= 0 && x <= W - 1)
            line(out, Point(cvRound(x), yBottom), Point(cvRound(x), yTop),
                color, std::max(1, cvRound(3 * s)), LINE_AA);
        };
    tick(d.carCenter, Scalar(0, 255, 255));
    tick(d.laneCenter, Scalar(0, 255, 0));
    drawWarningBorder();
    return out;
}
