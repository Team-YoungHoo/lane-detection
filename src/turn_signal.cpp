const int SIGNAL_OFF = 0;
const int SIGNAL_LEFT = 1;
const int SIGNAL_RIGHT = 2;

static int currentSignal = SIGNAL_OFF;
static int signalCount = 0;

void updateTurnSignal(int key)
{
    // 'a' 또는 'A' 키 입력 시 왼쪽 방향지시등 (90프레임 = 약 3초 유지)
    if (key == 'a' || key == 'A')
    {
        currentSignal = SIGNAL_LEFT;
        signalCount = 90;
    }
    else if (key == 'd' || key == 'D')
    {
        currentSignal = SIGNAL_RIGHT;
        signalCount = 90;
    }
    else if (key == 's' || key == 'S')
    {
        currentSignal = SIGNAL_OFF;
        signalCount = 0;
    }
    else if (currentSignal != SIGNAL_OFF)
    {
        signalCount--;

        if (signalCount <= 0)
        {
            currentSignal = SIGNAL_OFF;
        }
    }
}

int getTurnSignal()
{
    return currentSignal;
}