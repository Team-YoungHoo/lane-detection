#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
using namespace cv;
using namespace std;

Mat colorFilter(const Mat& frame);
Mat detectEdges(const Mat& mask);
Mat applyROI(const Mat& edges);
void drawROI(Mat& frame);
void setROIProfile(int idx);
void setLaneDisplayProfile(int idx); // overlay.cpp: video-specific display range
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
    double rxB, double rxT, double topRatio);
Mat drawLaneLinesOnly(const Mat& frame, bool lv, double lxB, double lxT,
    bool rv, double rxB, double rxT, double topRatio);
struct DepartInfo {
    double carCenter = 0, laneCenter = 0, laneWidth = 0, offset = 0;
    int state = 0;
    bool valid = false;
};
DepartInfo checkDeparture(int W, double lXb, double rXb);
Mat drawDeparture(const Mat& frame, const DepartInfo& d);

int main()
{
    vector<string> videos = {
        "C:\\COMPUTER_VISION\\lane-detection\\ROI\\Video\\clip1.mp4",
        "C:\\COMPUTER_VISION\\lane-detection\\ROI\\Video\\clip2.mp4",
        "C:\\COMPUTER_VISION\\lane-detection\\ROI\\Video\\clip3.mp4",
        "C:\\COMPUTER_VISION\\lane-detection\\ROI\\Video\\clip4.mp4",
        "C:\\COMPUTER_VISION\\lane-detection\\ROI\\Video\\videoplayback_(1).mp4"
    };
    int cur = 0;
    // Matches the topY values of the attached roi.cpp.
    const double TOP_RATIO[5] = { 0.60, 0.60, 0.60, 0.45, 0.52 };
    const Size procSize(640, 360);
    const int NW = 10;
    const char* wins[NW] = { "Original", "Color Mask", "Edges", "ROI Edges",
        "Hough Lines", "Filtered Lines", "Representative Lines",
        "Smoothed Lines", "Lane Result", "Departure" };
    const int showW = 427, showH = 240;
    for (int i = 0; i < NW; ++i) {
        namedWindow(wins[i], WINDOW_NORMAL);
        resizeWindow(wins[i], showW, showH);
        moveWindow(wins[i], (i % 4) * (showW + 10), (i / 4) * (showH + 40));
    }
    VideoCapture cap;
    VideoWriter writer;
    bool recording = true;
    double fps = 30;
    auto openVideo = [&](int idx) -> bool {
        if (!cap.open(videos[idx])) return false;
        fps = cap.get(CAP_PROP_FPS);
        if (fps <= 0 || fps > 240) fps = 30;
        setROIProfile(idx);
        setLaneDisplayProfile(idx);
        resetLane();
        return true;
        };
    if (!openVideo(cur)) {
        cout << "Cannot open video: " << videos[cur] << endl;
        return -1;
    }
    Mat frame;
    while (true) {
        int64 t0 = getTickCount();
        if (!cap.read(frame) || frame.empty()) {
            if (recording && writer.isOpened()) writer.release();
            recording = false;
            cap.set(CAP_PROP_POS_FRAMES, 0);
            resetLane();
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
        Mat result = ready ? drawResult(frame, lXb, lXt, rXb, rXt, tr) : frame.clone();
        DepartInfo dep;
        if (ready) dep = checkDeparture(frame.cols, lXb, rXb);
        Mat depView = drawDeparture(result, dep);
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
        imshow("Departure", depView);
        if (recording && writer.isOpened()) writer.write(depView);
        double elapsedMs = (getTickCount() - t0) * 1000.0 / getTickFrequency();
        int delay = max(1, (int)(1000.0 / fps - elapsedMs));
        int key = waitKey(delay);
        if (key == 27) break;
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
    return 0;
}
