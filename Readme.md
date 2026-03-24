 Custom Qt Webcam Suite (v5.14.1)
A lightweight C++ application built with Qt 5.14 that provides real-time camera preview, high-quality snapshots, and video recording capabilities.
🛠 The Architecture (The "FFmpeg Hack")
Standard Qt Multimedia recording (QMediaRecorder) often fails on Windows due to missing DirectShow/WMF codecs, resulting in an empty supportedContainers() list and an immediate StoppedState error.
To solve this, this project uses a Manual Frame-Stitching Engine:
1.	Capture: A QTimer triggers QCameraImageCapture every 100ms to save raw JPEGs to the system %TEMP% folder.
2.	Encoding: Upon stopping, the app utilizes FFmpeg via QProcess to stitch these frames into a high-quality H.264 .mp4 video.
3.	Storage: Final videos are timestamped and saved directly to the user's Desktop.
________________________________________
🏗 How to Build & Run
1. Prerequisites
●	Qt SDK 5.14.1 (MinGW or MSVC).
●	Qt Modules: Ensure multimedia and multimediawidgets are installed.
●	FFmpeg: This project requires the FFmpeg binary to encode video.
2. Setting up FFmpeg
Since the app calls FFmpeg via the system command line, you must:
1.    Crucial: Place ffmpeg.exe in the same folder as your compiled .exe (e.g., inside your debug or release build folder).
3. Compilation
1.	Open the .pro file in Qt Creator.
2.	Run qmake.
3.	Build (Ctrl+B) and Run (Ctrl+R).
4. For application usage find screenshots directory also.
________________________________________
🕹 Features
●	Hardware Detection: Automatically populates a list of available system cameras.
●	Live Viewfinder: Real-time preview with a "Blank Screen" fix upon stopping.
●	Robust Snapshots: Instant JPEG capture to the Desktop.
●	Fail-Safe Recording: Bypasses broken Windows media plugins using the FFmpeg pipeline.
●	Smart Cleanup: Automatically deletes temporary frames after the video is stitched to save disk space.
