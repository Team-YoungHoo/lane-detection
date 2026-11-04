// 방향지시등 상태 상수
const int SIGNAL_OFF = 0;
const int SIGNAL_LEFT = 1;
const int SIGNAL_RIGHT = 2;

// TurnSignal.cpp 내부에서만 접근 가능한 상태 변수
static int currentSignal = SIGNAL_OFF;
static int signalCount = 0;

// 방향지시등 상태 갱신 함수
void updateTurnSignal(int key)
{
    // 'a' 또는 'A' 키 입력 시 왼쪽 방향지시등 (90프레임 = 약 3초 유지)
    if (key == 'a' || key == 'A')
    {
        currentSignal = SIGNAL_LEFT;
        signalCount = 90;
    }
    // 'd' 또는 'D' 키 입력 시 오른쪽 방향지시등
    else if (key == 'd' || key == 'D')
    {
        currentSignal = SIGNAL_RIGHT;
        signalCount = 90;
    }
    // 's' 또는 'S' 키 입력 시 즉시 끄기
    else if (key == 's' || key == 'S')
    {
        currentSignal = SIGNAL_OFF;
        signalCount = 0;
    }
    // 지시등이 켜진 상태에서 키 입력이 없는 프레임은 카운트 감소
    else if (currentSignal != SIGNAL_OFF)
    {
        signalCount--;

        // 90프레임 경과 시 자동 OFF
        if (signalCount <= 0)
        {
            currentSignal = SIGNAL_OFF;
        }
    }
}

// 현재 방향지시등 상태 반환 함수
int getTurnSignal()
{
    return currentSignal;
}