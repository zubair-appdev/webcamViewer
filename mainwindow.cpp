#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Web Cam Viewer");

    populateCameras();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateCameras()
{
    // 1. Clear existing items to avoid duplicates if called multiple times
    ui->comboBox_cameras->clear();

    // 2. Fetch the list of all system-detected cameras
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();

    if (cameras.isEmpty()) {
        ui->comboBox_cameras->addItem("No cameras detected");
        return;
    }

    // 3. Loop through and add to the UI
    for (const QCameraInfo &cameraInfo : cameras) {
        // Display the user-friendly name, but store the unique deviceName internally
        ui->comboBox_cameras->addItem(cameraInfo.description(), cameraInfo.deviceName());
    }

    // 4. Set the selection to the system's default camera
    QCameraInfo defaultCam = QCameraInfo::defaultCamera();
    int index = ui->comboBox_cameras->findData(defaultCam.deviceName());
    if (index != -1) {
        ui->comboBox_cameras->setCurrentIndex(index);
    }
}


void MainWindow::on_pushButton_camerasRefresh_clicked()
{
    populateCameras();
}

void MainWindow::on_pushButton_startPreview_clicked()
{
    if (m_camera) {
        m_camera->stop();
        m_camera->unload(); // Force release the hardware
        delete m_camera;
        m_camera = nullptr;
    }

    if (m_mediaRecorder) {
        m_mediaRecorder->stop();
        delete m_mediaRecorder;
        m_mediaRecorder = nullptr;
    }
    if (m_imageCapture) {
        delete m_imageCapture;
        m_imageCapture = nullptr;
    }

    QByteArray deviceName = ui->comboBox_cameras->currentData().toByteArray();
    m_camera = new QCamera(deviceName, this);

    m_imageCapture = new QCameraImageCapture(m_camera);

    m_mediaRecorder = new QMediaRecorder(m_camera);

    // Optional: Connect a lambda to tell the user when the file is actually finished saving
    connect(m_mediaRecorder, &QMediaRecorder::stateChanged, this, [this](QMediaRecorder::State state){
        if (state == QMediaRecorder::StoppedState) {
            QMessageBox::information(this, "Video Saved", "Your clip has been saved successfully.");
        }
    });

    connect(m_mediaRecorder, QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error),
            this, [this](QMediaRecorder::Error) {
        QMessageBox::critical(this, "Recorder Error", m_mediaRecorder->errorString());
    });

    // Using the static_cast fix we discussed:
    auto errorSignal = static_cast<void (QCamera::*)(QCamera::Error)>(&QCamera::error);
    connect(m_camera, errorSignal, this, &MainWindow::handleCameraError);

    m_camera->setViewfinder(ui->videoWidget_preview);
    m_camera->start();
}


void MainWindow::on_pushButton_stopPreview_clicked()
{
    if (m_camera) {
        // 1. Stop the stream
        m_camera->stop();

        // 2. Disconnect the widget (This clears the last frozen frame)
        m_camera->setViewfinder(static_cast<QVideoWidget*>(nullptr));

        // 3. Optional: Trigger a repaint to ensure it's black/empty
        ui->videoWidget_preview->update();
    }
}

void MainWindow::handleCameraError(QCamera::Error error)
{
    QString errorMsg;

    switch (error) {
    case QCamera::CameraError:
        // In 5.14, this is the catch-all for hardware/access issues
        errorMsg = "A camera error occurred. Please check if another app is using the camera or if it is disconnected.";
        break;
    case QCamera::InvalidRequestError:
        errorMsg = "The system does not support the requested camera features.";
        break;
    case QCamera::ServiceMissingError:
        errorMsg = "The camera service is missing or not installed.";
        break;
    case QCamera::NotSupportedFeatureError:
        errorMsg = "This camera feature is not supported by your hardware.";
        break;
    default:
        errorMsg = QString("Camera error code: %1").arg(error);
        break;
    }

    QMessageBox::critical(this, "Camera Error", errorMsg);
}

#include <QMessageBox>
#include <QEventLoop>

void MainWindow::on_pushButton_snapshot_clicked()
{
    // 1. Safety check
    if (!m_camera || m_camera->state() != QCamera::ActiveState) {
        QMessageBox::warning(this, "Error", "Camera is not active!");
        return;
    }

    // 2. Prepare the capture object if it doesn't exist
    if (!m_imageCapture) {
        m_imageCapture = new QCameraImageCapture(m_camera);
    }

    // 3. The "Inline" Feedback (Lambda)
    // We connect it once. Note: Use Qt::UniqueConnection to avoid multiple popups
    // if the button is clicked multiple times.
    connect(m_imageCapture, &QCameraImageCapture::imageSaved, this,
            [this](int id, const QString &fileName) {
        Q_UNUSED(id);
        QMessageBox::information(this, "Success", "Saved to: " + fileName);
    }, Qt::UniqueConnection);


    // 4. Trigger the hardware
    m_camera->setCaptureMode(QCamera::CaptureStillImage);
    m_camera->searchAndLock();
    m_imageCapture->capture(); // This runs in the background
    m_camera->unlock();
}

void MainWindow::on_pushButton_startClip_clicked()
{
    // 1. Safety Check: Is the camera active?
    if (!m_camera || m_camera->state() != QCamera::ActiveState) {
        QMessageBox::warning(this, "Camera Error",
                             "Please start the camera preview before you begin recording.");
        return;
    }

    // 2. Prepare/Clean Temp Folder
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/webcam_temp";
    QDir dir(tempPath);
    if (dir.exists()) {
        dir.removeRecursively();
    }
    QDir().mkpath(tempPath);

    // 3. Reset Counter and Timer
    m_frameCounter = 0;
    if (m_recordTimer) m_recordTimer->deleteLater();

    m_recordTimer = new QTimer(this);
    connect(m_recordTimer, &QTimer::timeout, this, [this, tempPath](){
        if (m_imageCapture && m_imageCapture->isReadyForCapture()) {
            QString fileName = QString("/frame_%1.jpg").arg(m_frameCounter++, 4, 10, QChar('0'));
            m_imageCapture->capture(tempPath + fileName);
        }
    });

    // 4. Start the "Pulse"
    m_recordTimer->start(100); // 10 FPS

    // 5. Update UI
    ui->pushButton_startClip->setEnabled(false);
    ui->pushButton_stopClip->setEnabled(true);
}

void MainWindow::on_pushButton_stopClip_clicked()
{
    // 1. Stop the capture immediately
    if (m_recordTimer) {
        m_recordTimer->stop();
        m_recordTimer->deleteLater();
        m_recordTimer = nullptr;
    }

    // 2. Minimum frame check (FFmpeg needs at least 2 frames to make a video)
    if (m_frameCounter < 5) {
        QMessageBox::warning(this, "Recording Too Short",
                             "Please record for at least a few seconds.");
        ui->pushButton_startClip->setEnabled(true);
        ui->pushButton_stopClip->setEnabled(false);
        return;
    }

    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/webcam_temp";
    QString outputPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) +
                         "/Webcam_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mp4";

    // 3. Configure FFmpeg Process
    QProcess *ffmpeg = new QProcess(this);
    QStringList arguments;
    arguments << "-y" << "-framerate" << "10"
              << "-i" << tempPath + "/frame_%04d.jpg"
              << "-c:v" << "libx264" << "-pix_fmt" << "yuv420p"
              << outputPath;

    // 4. Handle "File Not Found" (If ffmpeg.exe is missing)
    connect(ffmpeg, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        if (error == QProcess::FailedToStart) {
            QMessageBox::critical(this, "System Error",
                "ffmpeg.exe not found in application folder! Video could not be created.");
        }
    });

    // 5. Final Result Popup
    connect(ffmpeg, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, tempPath, outputPath](int exitCode){
        if(exitCode == 0) {
            QMessageBox::information(this, "Success", "Video saved to Desktop:\n" + outputPath);
            QDir(tempPath).removeRecursively(); // Clean up the temp JPGs
        } else {
            QMessageBox::warning(this, "Encoding Failed", "FFmpeg encountered an error during stitching.");
        }
    });

    // 6. Launch & Reset Buttons
    ffmpeg->start("ffmpeg", arguments);
    ui->pushButton_startClip->setEnabled(true);
    ui->pushButton_stopClip->setEnabled(false);
}
