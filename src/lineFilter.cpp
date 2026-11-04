// =====================================================================
// lineFilter.cpp  -  3단계 ⑤ 직선 필터링 (좌/우 분리)
// ---------------------------------------------------------------------
// 역할 : 허프 변환이 찾은 선분 중 차선이 아닌 것(거의 수평인 그림자·정지선,
//        거의 수직인 기둥 등)을 제거하고, 왼쪽 차선 후보와 오른쪽 차선 후보로 나눈다.
// 입력 : 선분 목록, 프레임 너비
// 출력 : 왼쪽 선 목록, 오른쪽 선 목록
// 호출 : main.cpp  (separateLines, drawFilteredLines)
// 원리 : 이미지는 y가 아래로 커지므로
//          왼쪽 차선  = 기울기 음수 ( \ 모양) 이고 화면 중앙보다 왼쪽
//          오른쪽 차선 = 기울기 양수 ( / 모양) 이고 화면 중앙보다 오른쪽
// =====================================================================
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

// ===== 기울기 범위: |기울기|가 이 범위 안인 선만 차선으로 인정 =====
const double SLOPE_MIN = 0.5;   // 이보다 완만하면(거의 수평) 제거
const double SLOPE_MAX = 2.0;   // 이보다 가파르면(거의 수직) 제거

// [3-⑤] lines를 기울기·위치로 걸러 왼쪽/오른쪽으로 분리
void separateLines(const vector<Vec4i>& lines, int width,
    vector<Vec4i>& leftLines, vector<Vec4i>& rightLines)
{
    leftLines.clear();
    rightLines.clear();
    double centerX = width / 2.0;   // 화면 중앙 x

    for (const Vec4i& l : lines) {
        int x1 = l[0], y1 = l[1], x2 = l[2], y2 = l[3];
        if (x2 == x1) continue;                          // 수직선은 기울기 계산 불가 → 건너뜀

        double slope = (double)(y2 - y1) / (x2 - x1);
        double absS = fabs(slope);
        if (absS < SLOPE_MIN || absS > SLOPE_MAX) continue;  // 수평/수직에 가까운 선 제거

        double midX = (x1 + x2) / 2.0;                   // 선분 중점의 x 위치

        if (slope < 0 && midX < centerX)       leftLines.push_back(l);
        else if (slope > 0 && midX > centerX)  rightLines.push_back(l);
        // 기울기 부호와 위치가 맞지 않는 선은 버림
    }
}

// 확인용: 왼쪽 선은 파란색, 오른쪽 선은 빨간색으로 그린 영상 반환 (BGR 순서 주의)
Mat drawFilteredLines(const Mat& frame,
    const vector<Vec4i>& leftLines,
    const vector<Vec4i>& rightLines)
{
    Mat out = frame.clone();
    for (const Vec4i& l : leftLines)
        line(out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255, 0, 0), 3, LINE_AA);   // BGR 파랑
    for (const Vec4i& l : rightLines)
        line(out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 0, 255), 3, LINE_AA);   // BGR 빨강
    return out;
}