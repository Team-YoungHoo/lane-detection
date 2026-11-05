# Automatic Lane Detection for Smart Car

주행 영상에서 허프 변환으로 차선을 실시간 검출하고, 차선 이탈 시 경고하는 C++/OpenCV 프로젝트입니다. 방향지시등 키 입력으로 의도된 차선 변경과 의도치 않은 이탈을 구분합니다.

## 처리 과정

색상 필터 → Canny 에지 → ROI → `HoughLinesP` → 좌우 차선 분리 → 대표선 계산 → 평활화 → 이탈 판단 → 결과 표시

## 실행 환경

- Windows x64, Visual Studio 2026 (툴셋 v145), C++20
- OpenCV 4.12: Release는 공식 Windows 패키지(`opencv_world4120`), Debug는 `opencv_world4120d` 필요

## 빌드와 실행

1. `cv_project.vcxproj`의 `<OpenCvReleaseDir>`를 OpenCV의 `opencv/build` 경로로 바꿉니다.
2. `main.cpp`의 `videos` 목록을 영상 위치에 맞게 바꿉니다.
3. Visual Studio에서 폴더가 아닌 `cv_project.slnx`를 엽니다.
4. 구성을 **Release / x64**로 고르고 Ctrl+F5로 실행합니다.

Debug 빌드는 `OPENCV_DIR` 환경 변수가 필요하고, 실시간 속도가 나오지 않습니다.

## 조작 키

| 키 | 동작 |
| --- | --- |
| `1`\~`5` | 영상 전환 |
| `a` / `d` / `s` | 왼쪽 지시등 / 오른쪽 지시등 / 끄기 |
| `r` | 녹화 시작과 중지 (`demo_output` 폴더에 저장) |
| 스페이스바 | 일시정지 |
| `Esc` | 종료, 영상별 평균 FPS 출력 |

중간 처리 화면이 필요하면 `main.cpp`의 `SHOW_DEBUG_WINDOWS`를 `true`로 바꿉니다.

## 화면 상태

| 상태 | 조건 | 표시 |
| --- | --- | --- |
| Safe | 차선 검출, 이탈 아님 | 초록 차로 |
| Warning | 지시등 없이 또는 반대 방향으로 이탈 | 빨간 차로, 깜빡이는 테두리 |
| Lane change | 지시등과 같은 방향으로 이탈 | 노란 차로, 화살표 |
| Lane not detected | 차선 미검출, 이탈 중이 아님 | 차로 표시 없음 |

## 파일 구성

| 파일 | 역할 |
| --- | --- |
| `main.cpp` | 전체 루프, 키 입력, FPS, 녹화 |
| `color_edge.cpp` | 색상 필터, 에지 검출 |
| `roi.cpp` | 영상별 ROI |
| `hough.cpp` | 직선 후보 검출 |
| `lineFilter.cpp` | 좌우 차선 분리 |
| `laneFit.cpp` | 대표선 계산 |
| `smoothing.cpp` | 이상값 거부, 평활화 |
| `departure.cpp` | 오프셋, 상태 판단, 상태 표시 |
| `turn_signal.cpp` | 방향지시등 상태 |
| `overlay.cpp` | 차로 영역과 차선 표시 |

## 알아둘 점

- ROI와 차량 중심 보정값(`OFFSET_BIAS`)은 영상 5개에 맞춘 값이라 영상 순서가 바뀌거나 새 영상을 쓰면 다시 정해야 합니다.
- 이탈 여부는 실제 거리가 아니라 영상 좌표로 추정합니다.
- 횡단보도와 앞차 근처, 매우 어두운 구간에서는 오검출이나 미검출이 남아 있습니다.
