// =====================================================================
// laneFit.cpp  -  3단계 ⑥ 대표선 계산
// ---------------------------------------------------------------------
// 역할 : 한쪽(왼쪽 또는 오른쪽)에 모인 여러 선분을 직선 1개로 합친다.
// 입력 : 한쪽 선 목록, 프레임 높이 H, 대표선 상단 y 비율(topRatio)
// 출력 : 대표선의 아래쪽 x(xBottom), 위쪽 x(xTop), 성공 여부(bool)
//        y 좌표는 고정: 아래 y = H, 위 y = topRatio * H
// 호출 : main.cpp  (fitLane) - 왼쪽/오른쪽 각각 한 번씩 호출
// 원리 : 선분의 양 끝점을 전부 모아 fitLine(DIST_L2, 최소제곱)으로 직선을 구한다.
//        차선은 세로에 가까워 x = f(y) 꼴로 표현하는 편이 안정적이다.
//            x = x0 + (y - y0) * vx / vy
//        ((vx,vy)=방향벡터, (x0,y0)=직선이 지나는 점)
// 주의 : topRatio 는 ROI 상단 topY 와 같은 값을 넘겨야 대표선이 ROI 안에서 끝난다.
// =====================================================================
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

// [3-⑥] 한쪽 선들로부터 대표선 1개를 구한다. 성공하면 true.
bool fitLane(const vector<Vec4i>& lines, int H, double topRatio,
    double& xBottom, double& xTop)
{
    if (lines.empty()) return false;   // 선이 하나도 없으면 실패(미검출)

    // 모든 선분의 양 끝점을 한 점 목록으로 모음
    vector<Point2f> pts;
    for (const Vec4i& l : lines) {
        pts.push_back(Point2f((float)l[0], (float)l[1]));
        pts.push_back(Point2f((float)l[2], (float)l[3]));
    }

    Vec4f f;   // fitLine 결과: vx, vy, x0, y0
    fitLine(pts, f, DIST_L2, 0, 0.01, 0.01);
    float vx = f[0], vy = f[1], x0 = f[2], y0 = f[3];
    if (fabs(vy) < 1e-6f) return false;   // 수평선이면 x=f(y)를 못 구하므로 실패

    xBottom = x0 + (H - y0) * vx / vy;             // y = H (화면 맨 아래)에서의 x
    xTop = x0 + (topRatio * H - y0) * vx / vy;   // y = topRatio*H (ROI 상단)에서의 x
    return true;
}