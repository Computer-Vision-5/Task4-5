#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include "../backend/FaceRecognizer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void detectImage();      // detection-only: green boxes, no identity (no model needed)
    void recognizeImage();   // full recognition: blue box + name (requires trained model)
    void trainModel();
    void loadExistingModel();
    void evaluateModel();

private:
    QPushButton  *detectButton;   // always enabled — runs FaceDetector only
    QPushButton  *uploadButton;   // enabled after model is trained/loaded — runs FaceRecognizer
    QPushButton  *trainButton;
    QPushButton  *evalButton;
    QLabel       *imageLabel;
    QLabel       *resultLabel;
    QProgressBar *progressBar;

    FaceRecognizer m_recognizer;
    QString        m_trainPath;
};

#endif // MAINWINDOW_H