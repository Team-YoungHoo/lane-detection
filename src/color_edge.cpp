// 사용법 (다른 파일에서):
//   Mat colorFilter(const Mat& frame);   // 선언을 추가해서 사용
//   Mat detectEdges(const Mat& mask);
//
//   Mat mask  = colorFilter(frame);      // frame: 640x360 BGR 컬러 프레임
//   Mat edges = detectEdges(mask);       // mask : 차선 후보만 255인 이진 영상

#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// (const 전역 변수는 이 파일 안에서만 보이므로 다른 파일과 이름이 겹쳐도 괜찮음)

// 흰색 A: 밝은 영상용 (V 높고, S 낮고, 주변보다 약간 밝음)
const int WHITE_V_MIN = 125, WHITE_S_MAX = 50, WHITE_DIFF_MIN = 20;
// 흰색 B: 어두운 영상용 (V는 낮아도 주변보다 확실히 밝음)
const int DARK_V_MIN = 30, DARK_DIFF_MIN = 25;
// 노란색 (OpenCV H: 0~180)
const int Y_H_MIN = 15, Y_H_MAX = 35, Y_S_MIN = 65, Y_V_MIN = 138;
// 파란색: 두 조건 중 하나만 맞으면 통과
const int B_H_MIN = 85, B_H_MAX = 108;
const int B_S_MIN = 40, B_V_MIN = 120;         // 밝은 파랑
const int VIVID_S_MIN = 90, VIVID_V_MIN = 60;  // 선명한 파랑 (어두워도 통과)
// 주변 밝기를 구하는 블러 크기 (640x360 기준)
const int BG_BLUR = 41;
// Canny (낮은 값 : 높은 값 = 1 : 3)
const int CANNY_LOW = 50, CANNY_HIGH = 150;

// ch >= v 인 곳만 255
static Mat atLeast(const Mat& ch, int v)
{
    Mat m;
    threshold(ch, m, v - 1, 255, THRESH_BINARY);
    return m;
}

// ch <= v 인 곳만 255
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
    split(hsv, ch); // ch[0]=H, ch[1]=S, ch[2]=V
    Mat hue = ch[0], sat = ch[1], val = ch[2];

    // 주변보다 밝은 정도 = 밝기 - 주변 평균 밝기 (원본에서 저주파를 뺀 고주파 성분)
    Mat bg, diff, tmp;
    blur(val, bg, Size(BG_BLUR, BG_BLUR));     // 평균 필터
    diff = val - bg;                           // 음수는 0으로 처리

    // 흰색 A
    Mat whiteA = atLeast(val, WHITE_V_MIN);
    bitwise_and(whiteA, atMost(sat, WHITE_S_MAX), whiteA);
    threshold(diff, tmp, WHITE_DIFF_MIN, 255, THRESH_BINARY);
    bitwise_and(whiteA, tmp, whiteA);

    // 흰색 B
    Mat whiteB = atLeast(val, DARK_V_MIN);
    bitwise_and(whiteB, atMost(sat, WHITE_S_MAX), whiteB);
    threshold(diff, tmp, DARK_DIFF_MIN, 255, THRESH_BINARY);
    bitwise_and(whiteB, tmp, whiteB);

    // 노란색
    Mat yellow = atLeast(hue, Y_H_MIN);
    bitwise_and(yellow, atMost(hue, Y_H_MAX), yellow);
    bitwise_and(yellow, atLeast(sat, Y_S_MIN), yellow);
    bitwise_and(yellow, atLeast(val, Y_V_MIN), yellow);

    // 파란색
    Mat blueH = atLeast(hue, B_H_MIN);
    bitwise_and(blueH, atMost(hue, B_H_MAX), blueH);

    Mat blueBright = atLeast(sat, B_S_MIN);
    bitwise_and(blueBright, atLeast(val, B_V_MIN), blueBright);
    bitwise_and(blueBright, blueH, blueBright);

    Mat blueVivid = atLeast(sat, VIVID_S_MIN);
    bitwise_and(blueVivid, atLeast(val, VIVID_V_MIN), blueVivid);
    bitwise_and(blueVivid, blueH, blueVivid);

    // 합치기
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