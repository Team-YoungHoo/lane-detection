// =====================================================================
// roi.cpp  -  3단계 ③ ROI 마스크
// ---------------------------------------------------------------------
// 역할 : 차선이 있을 수 있는 영역(사다리꼴)만 남기고 나머지(하늘, 옆 차로,
//        가드레일, 자기 차 보닛 등)는 검게 지운다.
// 입력 : 에지 이미지 (1채널)
// 출력 : 사다리꼴 안쪽의 에지만 남은 이미지
// 호출 : main.cpp  (applyROI, drawROI, setROIProfile)
// 주의 : 영상마다 ROI 모양이 다르므로 PROFILES 표에 영상별 값을 둔다.
//        ★ 각 프로필의 topY 값은 main.cpp 의 TOP_RATIO 배열과 반드시 같아야 한다.
//          (대표선 상단 y를 ROI 상단과 맞추기 위해 사용)
// =====================================================================
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

// ROI 파라미터. 모든 값은 프레임 너비(W)/높이(H) 대비 비율(0~1)
struct ROIParam {
    double botLeftX, botRightX, botY;   // 사다리꼴 아래 변: 왼쪽 x, 오른쪽 x, y
    double topLeftX, topRightX, topY;   // 사다리꼴 위 변  : 왼쪽 x, 오른쪽 x, y
};

// 영상 번호별 ROI (인덱스 0~4 = 키보드 1~5번 영상)
static const ROIParam PROFILES[5] = {
    // botL  botR  botY  topL  topR  topY
    { 0.10, 0.78, 1.00, 0.40, 0.48, 0.60 },   // 1번 (지정값 그대로)
    { 0.15, 0.80, 1.00, 0.40, 0.50, 0.60 },   // 2번 (지정값 그대로)
    { 0.15, 0.80, 1.00, 0.40, 0.50, 0.60 },   // 3번 (지정값 그대로)
    { 0.13, 0.70, 0.75, 0.36, 0.52, 0.45 },   // 4번 (보닛 제외 + 위로 길게)
    { 0.25, 0.80, 0.80, 0.45, 0.52, 0.52 }    // 5번 (보닛 제외 + 위로 길게)
};

// 현재 사용 중인 ROI (이 파일 안에서만 보이는 전역 변수)
static ROIParam g_roi = PROFILES[0];

// 영상이 바뀔 때 main에서 호출: idx번 영상의 ROI로 교체
void setROIProfile(int idx)
{
    if (idx < 0 || idx > 4) idx = 0;   // 범위를 벗어나면 1번 영상 값 사용
    g_roi = PROFILES[idx];
}

// 비율 값을 실제 픽셀 좌표 4개(사다리꼴 꼭짓점)로 변환
// 순서: 왼쪽 아래 → 오른쪽 아래 → 오른쪽 위 → 왼쪽 위
static vector<Point> roiPolygon(Size sz)
{
    int W = sz.width, H = sz.height;
    int yBot = min(H - 1, (int)(H * g_roi.botY));   // 화면 밖으로 나가지 않게 제한
    return {
        Point((int)(W * g_roi.botLeftX),  yBot),
        Point((int)(W * g_roi.botRightX), yBot),
        Point((int)(W * g_roi.topRightX), (int)(H * g_roi.topY)),
        Point((int)(W * g_roi.topLeftX),  (int)(H * g_roi.topY))
    };
}

// [3-③] 에지 이미지에 ROI 마스크를 적용
// 방법: 검은 이미지에 사다리꼴을 흰색으로 채운 마스크를 만들고,
//       에지 이미지와 bitwise_and 하면 사다리꼴 안쪽만 남는다.
Mat applyROI(const Mat& edges)
{
    Mat roi = Mat::zeros(edges.size(), CV_8UC1);              // 전부 검정인 마스크
    vector<vector<Point>> polys = { roiPolygon(edges.size()) };
    fillPoly(roi, polys, Scalar(255));                        // 사다리꼴만 흰색
    Mat result;
    bitwise_and(edges, roi, result);                          // 마스크 안쪽 에지만 통과
    return result;
}

// 확인용: 원본 프레임 위에 ROI 사다리꼴 테두리를 초록색으로 그림 (frame을 직접 수정)
void drawROI(Mat& frame)
{
    vector<Point> poly = roiPolygon(frame.size());
    polylines(frame, poly, true, Scalar(0, 255, 0), 2);
}