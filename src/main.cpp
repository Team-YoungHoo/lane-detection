#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <ctime>
#include <filesystem>
using namespace cv;
using namespace std;

// 최종 시연 때는 false: Departure 창만 표시
const bool SHOW_DEBUG_WINDOWS = false;

Mat colorFilter(const Mat& frame);
Mat detectEdges(const Mat& mask);
Mat applyROI(const Mat& edges);
void drawROI(Mat& frame);
void setROIProfile(int idx);
void setLaneDisplayProfile(int idx);
vector<Vec4i> detectLines(const Mat& roiEdges);
Mat drawAllLines(const Mat& frame, const vector<Vec4i>& lines);
void separateLines(const vector<Vec4i>& lines, int width,
    vector<Vec4i>& leftLines, vector<Vec4i>& rightLines);
Mat drawFilteredLines(const Mat& frame, const vector<Vec4i>& leftLines,
    const vector<Vec4i>& rightLines);
bool fitLane(const vector<Vec4i>& lines, int H, double topRatio,
    double& xBottom, double& xTop);
void resetLane();
bool smoothLane(bool nlv, double nLxb, double nLxt,
    bool nrv, double nRxb, double nRxt, int W,
    double& lXb, double& lXt, double& rXb, double& rXt);
Mat drawResult(const Mat& frame, double lxB, double lxT,
    double rxB, double rxT, double topRatio, const Scalar& fillColor);
Mat drawLaneLinesOnly(const Mat& frame, bool lv, double lxB, double lxT,
    bool rv, double rxB, double rxT, double topRatio);
// departure.cpp의 정의와 동일하게 유지
enum LaneState { ST_SAFE, ST_WARNING, ST_CHANGE, ST_LOST };
struct DepartInfo {
    double carCenter = 0, laneCenter = 0, laneWidth = 0, offset = 0;
    int state = ST_LOST;
    bool valid = false;
    int direction = 0;
    bool held = false;
};
Scalar laneStateColor(int state);
void setDepartureProfile(int idx);
DepartInfo updateDeparture(bool ready, int W, double lXb, double rXb, int signal);
Mat drawDeparture(const Mat& frame, const DepartInfo& d, int signal, int frameNo);
void updateTurnSignal(int key);
int getTurnSignal();

static string makeDemoPath(int videoNo)
{
    filesystem::create_directories("demo_output");
    const time_t now = time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    char name[64];
    snprintf(name, sizeof(name), "demo_video%d_%02d%02d%02d.mp4", videoNo,
        local.tm_hour, local.tm_min, local.tm_sec);
    return filesystem::absolute(filesystem::path("demo_output") / name).string();
}

int main()
{
    vector<string> videos = {
        "C:\\Users\\Administrator\\Desktop\\Current_Semester\\COMPUTER_VISION\\projects\\Video\\clip1.mp4",
        "C:\\Users\\Administrator\\Desktop\\Current_Semester\\COMPUTER_VISION\\projects\\Video\\clip2.mp4",
        "C:\\Users\\Administrator\\Desktop\\Current_Semester\\COMPUTER_VISION\\projects\\Video\\clip3.mp4",
        "C:\\Users\\Administrator\\Desktop\\Current_Semester\\COMPUTER_VISION\\projects\\Video\\clip4.mp4",
        "C:\\Users\\Administrator\\Desktop\\Current_Semester\\COMPUTER_VISION\\projects\\Video\\videoplayback_(1).mp4",
    };
    int cur = 0;
    // roi.cpp의 topY와 같아야 함
    const double TOP_RATIO[5] = { 0.60, 0.60, 0.60, 0.45, 0.52 };
    const Size procSize(640, 360);
    const int NW = 10;
    const char* wins[NW] = { "Original", "Color Mask", "Edges", "ROI Edges",
        "Hough Lines", "Filtered Lines", "Representative Lines",
        "Smoothed Lines", "Lane Result", "Departure" };
    const int showW = 427, showH = 240;
    for (int i = 0; i < NW; ++i) {
        if (!SHOW_DEBUG_WINDOWS && i != NW - 1) continue;
        namedWindow(wins[i], WINDOW_NORMAL);
        resizeWindow(wins[i], showW, showH);
        moveWindow(wins[i], (i % 4) * (showW + 10), (i / 4) * (showH + 40));
    }
    VideoCapture cap;
    VideoWriter writer;
    string demoPath;
    double fps = 30;
    auto stopRecording = [&]() {
        if (!writer.isOpened()) return;
        writer.release();
        cout << "Recording saved: " << demoPath << endl;
        };
    auto startRecording = [&](int idx) {
        demoPath = makeDemoPath(idx + 1);
        if (writer.open(demoPath, VideoWriter::fourcc('m', 'p', '4', 'v'), fps, procSize))
            cout << "Recording started: " << demoPath << endl;
        else
            cout << "Cannot open video writer: " << demoPath << endl;
        };
    double processSec[5] = {}, displaySec[5] = {}, playbackSec[5] = {};
    const double tickFreq = getTickFrequency();
    int64 nextFrameTick = getTickCount();
    int timedFrames[5] = {};
    double shownFps = 0;
    TickMeter frameTimer;
    auto openVideo = [&](int idx) -> bool {
        stopRecording();  // 영상마다 FPS가 다를 수 있어 영상별로 파일을 나눈다.
        if (!cap.open(videos[idx])) return false;
        fps = cap.get(CAP_PROP_FPS);
        if (fps <= 0 || fps > 240) fps = 30;
        setROIProfile(idx);
        setLaneDisplayProfile(idx);
        setDepartureProfile(idx);
        resetLane();
        updateTurnSignal('s');
        nextFrameTick = getTickCount();
        return true;
        };
    if (!openVideo(cur)) {
        cout << "Cannot open video: " << videos[cur] << endl;
        return -1;
    }
    Mat frame;
    int frameNo = 0;
    while (true) {
        int64 t0 = getTickCount();
        frameTimer.reset();
        frameTimer.start();
        if (!cap.read(frame) || frame.empty()) {
            stopRecording();
            cap.set(CAP_PROP_POS_FRAMES, 0);
            resetLane();
            setDepartureProfile(cur);
            continue;
        }
        resize(frame, frame, procSize);
        Mat mask = colorFilter(frame);
        Mat edges = detectEdges(mask);
        Mat roiEdge = applyROI(edges);
        vector<Vec4i> lines = detectLines(roiEdge);
        vector<Vec4i> leftLines, rightLines;
        separateLines(lines, frame.cols, leftLines, rightLines);
        double tr = TOP_RATIO[cur];
        double nLxb = 0, nLxt = 0, nRxb = 0, nRxt = 0;
        bool nlv = fitLane(leftLines, frame.rows, tr, nLxb, nLxt);
        bool nrv = fitLane(rightLines, frame.rows, tr, nRxb, nRxt);
        double lXb = 0, lXt = 0, rXb = 0, rXt = 0;
        bool ready = smoothLane(nlv, nLxb, nLxt, nrv, nRxb, nRxt, frame.cols,
            lXb, lXt, rXb, rXt);
        const int signal = getTurnSignal();
        DepartInfo dep = updateDeparture(ready, frame.cols, lXb, rXb, signal);
        Mat result = !dep.valid ? frame.clone()
            : drawResult(frame, lXb, lXt, rXb, rXt, tr, laneStateColor(dep.state));
        Mat depView = drawDeparture(result, dep, signal, frameNo++);

        frameTimer.stop();
        const double currentFps = frameTimer.getFPS();
        shownFps = shownFps > 0 ? 0.9 * shownFps + 0.1 * currentFps : currentFps;
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", shownFps);
        putText(depView, fpsText, Point(frame.cols - 130, 28), FONT_HERSHEY_SIMPLEX,
            0.59, Scalar(255, 255, 255), 2, LINE_AA);

        // write() 뒤에 그려서 REC 표시는 저장 영상에 들어가지 않는다.
        if (writer.isOpened()) {
            writer.write(depView);
            circle(depView, Point(frame.cols - 118, 48), 6, Scalar(0, 0, 255), FILLED, LINE_AA);
            putText(depView, "REC", Point(frame.cols - 106, 54), FONT_HERSHEY_SIMPLEX,
                0.5, Scalar(0, 0, 255), 2, LINE_AA);
        }

        if (SHOW_DEBUG_WINDOWS) {
            Mat view = frame.clone();
            drawROI(view);
            imshow("Original", view);
            imshow("Color Mask", mask);
            imshow("Edges", edges);
            imshow("ROI Edges", roiEdge);
            imshow("Hough Lines", drawAllLines(frame, lines));
            imshow("Filtered Lines", drawFilteredLines(frame, leftLines, rightLines));
            imshow("Representative Lines", drawLaneLinesOnly(frame, nlv, nLxb, nLxt,
                nrv, nRxb, nRxt, tr));
            imshow("Smoothed Lines", drawLaneLinesOnly(frame, ready, lXb, lXt,
                ready, rXb, rXt, tr));
            imshow("Lane Result", result);
        }
        imshow("Departure", depView);
        double elapsedMs = (getTickCount() - t0) * 1000.0 / getTickFrequency();
        processSec[cur] += frameTimer.getTimeSec();
        displaySec[cur] += elapsedMs / 1000.0;
        ++timedFrames[cur];
        // waitKey가 더 오래 대기해도 오차가 쌓이지 않도록 절대 시각 기준으로 기다린다.
        const int64 framePeriod = (int64)(tickFreq / fps);
        nextFrameTick += framePeriod;
        const int64 now = getTickCount();
        if (now - nextFrameTick > 5 * framePeriod) nextFrameTick = now;
        const int delay = max(1, (int)((nextFrameTick - now) * 1000.0 / tickFreq));
        int key = waitKey(delay);
        playbackSec[cur] += (getTickCount() - t0) / tickFreq;
        if (key == ' ') {
            do { key = waitKey(0); } while (key != ' ' && key != 27);
            if (key == ' ') key = -1;
            nextFrameTick = getTickCount();
        }
        if (key == 27) break;
        updateTurnSignal(key);
        if (key == 'r' || key == 'R') {
            if (writer.isOpened()) stopRecording();
            else startRecording(cur);
        }
        if (key >= '1' && key <= '5') {
            int n = key - '1';
            if (n != cur) {
                if (openVideo(n)) cur = n;
                else {
                    cout << "Cannot open video: " << videos[n] << endl;
                    if (!openVideo(cur)) break;
                }
            }
        }
    }
    stopRecording();
    cout << "\n=== Average FPS ===" << endl;
    for (int i = 0; i < 5; ++i) {
        if (timedFrames[i] == 0) continue;
        char line[140];
        snprintf(line, sizeof(line), "Video %d: process %.1f FPS, with display %.1f FPS, playback %.1f FPS (%d frames)",
            i + 1, timedFrames[i] / processSec[i], timedFrames[i] / displaySec[i],
            timedFrames[i] / playbackSec[i], timedFrames[i]);
        cout << line << endl;
    }
    return 0;
}
