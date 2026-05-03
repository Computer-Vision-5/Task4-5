#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QButtonGroup>
#include <QString>
#include <QImage>
#include <QElapsedTimer>
#include <QEvent>
#include <QMouseEvent>
#include "../backend/include/Thresholding.h"
#include "../backend/include/Segmentation.h"
#include "../backend/include/image.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

protected:
    // This allows us to detect double-clicks on the image label
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void loadImage();
    void processImage();
    void onMethodChanged(int id);

private:
    // UI Elements
    QLabel *originalImageLabel;
    QLabel *processedImageLabel;
    QPushButton *processButton;
    QButtonGroup *methodGroup;

    // Parameter Controls
    QLabel *kLabel;
    QSpinBox *kSpinBox;
    QLabel *thresholdLabel;
    QSpinBox *thresholdSpinBox;
    QLabel *spatialRadiusLabel;
    QDoubleSpinBox *spatialRadiusSpinBox;
    QLabel *colorRadiusLabel;
    QDoubleSpinBox *colorRadiusSpinBox;
    QLabel *seedXLabel;
    QSpinBox *seedXSpinBox;
    QLabel *seedYLabel;
    QSpinBox *seedYSpinBox;

    // Status & Timing
    QLabel *statusLabel;
    QLabel *timeLabel;

    // Backend Data
    QImage loadedQImage;
    bool isImageLoaded = false;
    int currentMethodId = 0;

    // Conversion Helpers
    GrayImage qImageToGrayImage(const QImage& qimg);
    ColorImage qImageToColorImage(const QImage& qimg);
    QPixmap grayImageToQPixmap(const GrayImage& img);
    QPixmap colorImageToQPixmap(const ColorImage& img);
    
    // UI Builders
    void setupUI();
    void applyStyleSheet();
    QVBoxLayout* createParamLayout(QLabel* label, QWidget* editor);
};