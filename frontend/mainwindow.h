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
    void uploadImage();
    void trainModel();
    void loadExistingModel();

private:
    QPushButton  *uploadButton;
    QPushButton  *trainButton;
    QLabel       *imageLabel;
    QLabel       *resultLabel;
    QProgressBar *progressBar;

    FaceRecognizer m_recognizer;
    QString        m_trainPath;
};

#endif // MAINWINDOW_H