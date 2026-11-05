#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// 흰색 A: 밝은 영상용 (V 높고, S 낮고, 주변보다 약간 밝음)
const int WHITE_V_MIN = 125, WHITE_S_MAX = 50, WHITE_DIFF_MIN = 20;
// 흰색 B: 어두운 영상용 (V는 낮아도 주변보다 확실히 밝음)
const int DARK_V_MIN = 30, DARK_DIFF_MIN = 25;
const int Y_H_MIN = 15, Y_H_MAX = 35, Y_S_MIN = 65, Y_V_MIN = 138;
// 파란색: 두 조건 중 하나만 맞으면 통과
const int B_H_MIN = 85, B_H_MAX = 108;
const int B_S_MIN = 40, B_V_MIN = 120;
const int VIVID_S_MIN = 90, VIVID_V_MIN = 60;
const int BG_BLUR = 41;
const int CANNY_LOW = 50, CANNY_HIGH = 150;

static Mat atLeast(const Mat& ch, int v)
{
    Mat m;
    threshold(ch, m, v - 1, 255, THRESH_BINARY);
    return m;
}

static Mat atMost(const Mat& ch, int v)
{
    Mat m;
    threshold(ch, m, v, 255, THRESH_BINARY_INV);
    return m;
}

Mat colorFilter(const Mat& frame)
{
    Mat hsv;
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    vector<Mat> ch;
    split(hsv, ch);
    Mat hue = ch[0], sat = ch[1], val = ch[2];

    Mat bg, diff, tmp;
    blur(val, bg, Size(BG_BLUR, BG_BLUR));
    diff = val - bg;                           // 음수는 0으로 처리

    Mat whiteA = atLeast(val, WHITE_V_MIN);
    bitwise_and(whiteA, atMost(sat, WHITE_S_MAX), whiteA);
    threshold(diff, tmp, WHITE_DIFF_MIN, 255, THRESH_BINARY);
    bitwise_and(whiteA, tmp, whiteA);

    Mat whiteB = atLeast(val, DARK_V_MIN);
    bitwise_and(whiteB, atMost(sat, WHITE_S_MAX), whiteB);
    threshold(diff, tmp, DARK_DIFF_MIN, 255, THRESH_BINARY);
    bitwise_and(whiteB, tmp, whiteB);

    Mat yellow = atLeast(hue, Y_H_MIN);
    bitwise_and(yellow, atMost(hue, Y_H_MAX), yellow);
    bitwise_and(yellow, atLeast(sat, Y_S_MIN), yellow);
    bitwise_and(yellow, atLeast(val, Y_V_MIN), yellow);

    Mat blueH = atLeast(hue, B_H_MIN);
    bitwise_and(blueH, atMost(hue, B_H_MAX), blueH);

    Mat blueBright = atLeast(sat, B_S_MIN);
    bitwise_and(blueBright, atLeast(val, B_V_MIN), blueBright);
    bitwise_and(blueBright, blueH, blueBright);

    Mat blueVivid = atLeast(sat, VIVID_S_MIN);
    bitwise_and(blueVivid, atLeast(val, VIVID_V_MIN), blueVivid);
    bitwise_and(blueVivid, blueH, blueVivid);

    Mat mask;
    bitwise_or(whiteA, whiteB, mask);
    bitwise_or(mask, yellow, mask);
    bitwise_or(mask, blueBright, mask);
    bitwise_or(mask, blueVivid, mask);
    return mask;
}

Mat detectEdges(const Mat& mask)
{
    Mat smooth, edges;
    GaussianBlur(mask, smooth, Size(5, 5), 0);
    Canny(smooth, edges, CANNY_LOW, CANNY_HIGH);
    return edges;
}