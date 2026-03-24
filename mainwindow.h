#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCameraInfo>
#include <QComboBox>
#include <QVideoWidget>
#include <QMessageBox>
#include <QCameraImageCapture>
#include <QMediaRecorder>
#include <QUrl>
#include <QStandardPaths>
#include <QDateTime> // Useful for unique filenames
#include <QTimer>
#include <QProcess>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void populateCameras();

private slots:
    void on_pushButton_camerasRefresh_clicked();

    void on_pushButton_startPreview_clicked();

    void on_pushButton_stopPreview_clicked();

    void handleCameraError(QCamera::Error error);

    void on_pushButton_snapshot_clicked();

    void on_pushButton_startClip_clicked();

    void on_pushButton_stopClip_clicked();

private:
    Ui::MainWindow *ui;
    QCamera *m_camera = nullptr;

    QCameraImageCapture *m_imageCapture = nullptr;
    QMediaRecorder *m_mediaRecorder = nullptr;

    QTimer *m_recordTimer = nullptr;
    int m_frameCounter = 0;
};
#endif // MAINWINDOW_H
