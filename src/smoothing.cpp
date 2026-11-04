// =====================================================================
// smoothing.cpp  -  3단계 ⑦ 프레임 간 평활화 + 이상값 거부
// ---------------------------------------------------------------------
// 역할 : 프레임마다 구한 대표선이 떨리거나 순간적으로 끊기지 않도록
//        1) 이전 값과 너무 다른 값은 거부하고 (이상값 거부)
//        2) 지수이동평균(EMA)으로 부드럽게 섞는다.
// 입력 : 이번 프레임의 좌/우 대표선(검출 여부 포함), 프레임 너비 W
// 출력 : 평활화된 좌/우 대표선의 하단 x, 상단 x. 둘 다 유효하면 true
// 호출 : main.cpp  (smoothLane, resetLane)
// 상태 : 이전 프레임의 값을 이 파일 안의 전역 변수(g_left, g_right)에 기억한다.
//        → 영상이 바뀌거나 반복 재생될 때 반드시 resetLane() 을 호출해야 한다.
// =====================================================================
#include <cmath>

using namespace std;

const double OUTLIER_X_RATIO = 0.1;   // 하단 x 가 이전 값과 0.1*W 이상 차이 나면 거부
const double EMA_ALPHA = 0.25;        // EMA 가중치 (0.2~0.3). 클수록 새 값에 빨리 반응
const int    MAX_MISS = 10;           // 연속 10프레임 이상 미검출이면 "미검출"로 처리

// 한쪽 차선의 상태 (이 파일 안에서만 보임)
struct Side {
    double xb = 0, xt = 0;   // 평활화된 하단/상단 x
    bool valid = false;      // 현재 쓸 수 있는 값이 있는지
    int miss = 0;            // 연속 미검출 프레임 수
};
static Side g_left, g_right;   // 왼쪽/오른쪽 차선의 이전 상태

// 평활화 상태 초기화. 영상이 바뀌거나 반복 재생될 때 호출
void resetLane()
{
    g_left = Side();
    g_right = Side();
}

// 이상값 판정: 새 값이 이전 값과 너무 다르면 거부 (이전 값이 없으면 무조건 통과)
static bool passJump(bool newValid, double newXb, const Side& s, int W)
{
    if (!newValid) return false;   // 이번 프레임에 검출 자체가 안 됨
    if (s.valid && fabs(newXb - s.xb) >= OUTLIER_X_RATIO * W) return false;   // 갑자기 튄 값
    return true;
}

// 상태 갱신
//  ok == true : 평균을 내서 갱신 (처음이면 새 값을 그대로 사용)
//  ok == false: 이전 값 유지 + 미검출 횟수 증가, MAX_MISS 이상이면 무효 처리
static void update(Side& s, bool ok, double nxb, double nxt)
{
    if (ok) {
        if (!s.valid) { s.xb = nxb; s.xt = nxt; s.valid = true; }
        else {
            // 현재값 = α × 새 값 + (1 − α) × 이전 값
            s.xb = EMA_ALPHA * nxb + (1 - EMA_ALPHA) * s.xb;
            s.xt = EMA_ALPHA * nxt + (1 - EMA_ALPHA) * s.xt;
        }
        s.miss = 0;
    }
    else if (s.valid && ++s.miss >= MAX_MISS) {
        s.valid = false;
    }
}

// [3-⑦] 이상값 거부 + 평활화
// nlv/nrv      : 이번 프레임에 왼쪽/오른쪽 대표선이 검출됐는지 (fitLane 의 반환값)
// nLxb, nLxt.. : 이번 프레임의 새 대표선 값 (하단 x, 상단 x)
// lXb, lXt, rXb, rXt : [출력] 평활화된 값
// 반환값       : 좌우 모두 유효하면 true (그릴 수 있음)
bool smoothLane(bool nlv, double nLxb, double nLxt,
    bool nrv, double nRxb, double nRxt, int W,
    double& lXb, double& lXt, double& rXb, double& rXt)
{
    bool okL = passJump(nlv, nLxb, g_left, W);
    bool okR = passJump(nrv, nRxb, g_right, W);

    // 왼쪽 하단 x가 오른쪽보다 크면(좌우가 뒤바뀜) 둘 다 거부
    if (nlv && nrv && nLxb > nRxb) okL = okR = false;

    update(g_left, okL, nLxb, nLxt);
    update(g_right, okR, nRxb, nRxt);

    lXb = g_left.xb;  lXt = g_left.xt;
    rXb = g_right.xb; rXt = g_right.xt;
    return g_left.valid && g_right.valid;
}