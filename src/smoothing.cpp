#include <cmath>

using namespace std;

const double OUTLIER_X_RATIO = 0.1;   // 하단 x 가 이전 값과 0.1*W 이상 차이 나면 거부
const double EMA_ALPHA = 0.2;
const int    MAX_MISS = 10;

struct Side {
    double xb = 0, xt = 0;
    bool valid = false;
    int miss = 0;
};
static Side g_left, g_right;

void resetLane()
{
    g_left = Side();
    g_right = Side();
}

static bool passJump(bool newValid, double newXb, const Side& s, int W)
{
    if (!newValid) return false;
    if (s.valid && fabs(newXb - s.xb) >= OUTLIER_X_RATIO * W) return false;
    return true;
}

static void update(Side& s, bool ok, double nxb, double nxt)
{
    if (ok) {
        if (!s.valid) { s.xb = nxb; s.xt = nxt; s.valid = true; }
        else {
            s.xb = EMA_ALPHA * nxb + (1 - EMA_ALPHA) * s.xb;
            s.xt = EMA_ALPHA * nxt + (1 - EMA_ALPHA) * s.xt;
        }
        s.miss = 0;
    }
    else if (s.valid && ++s.miss >= MAX_MISS) {
        s.valid = false;
    }
}

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