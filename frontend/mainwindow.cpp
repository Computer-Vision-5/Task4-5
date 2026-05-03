#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Computer Vision Toolkit - Task 4");
    resize(1200, 750);

    setupUI();
    applyStyleSheet();

    // Call once to set initial parameter visibility (Otsu is ID 0)
    onMethodChanged(0);
}

// Catches the double-click event on the image label
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == originalImageLabel && event->type() == QEvent::MouseButtonDblClick) {
        loadImage();
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

// Helper to stack a label neatly on top of a spinbox
QVBoxLayout* MainWindow::createParamLayout(QLabel* label, QWidget* editor) {
    QVBoxLayout* layout = new QVBoxLayout();
    label->setObjectName("paramLabel");
    layout->addWidget(label);
    layout->addWidget(editor);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(2);
    return layout;
}

void MainWindow::setupUI() {
    // --- 1. TOP BAR (PARAMETERS & CONTROLS) ---
    QGroupBox *controlsBox = new QGroupBox("Parameters and Controls", this);
    QHBoxLayout *topLayout = new QHBoxLayout();

    // -- Left: Parameters --
    kLabel = new QLabel("Clusters (k)");
    kSpinBox = new QSpinBox(); kSpinBox->setRange(2, 20); kSpinBox->setValue(4);

    thresholdLabel = new QLabel("Threshold");
    thresholdSpinBox = new QSpinBox(); thresholdSpinBox->setRange(1, 255); thresholdSpinBox->setValue(20);

    // Added Local Thresholding Parameters
    windowSizeLabel = new QLabel("Window Size");
    windowSizeSpinBox = new QSpinBox();
    windowSizeSpinBox->setRange(3, 99);
    windowSizeSpinBox->setSingleStep(2); // Keep it odd
    windowSizeSpinBox->setValue(15);

    cValueLabel = new QLabel("Constant C");
    cValueSpinBox = new QSpinBox();
    cValueSpinBox->setRange(-50, 50);
    cValueSpinBox->setValue(10);

    spatialRadiusLabel = new QLabel("Spatial Radius");
    spatialRadiusSpinBox = new QDoubleSpinBox(); spatialRadiusSpinBox->setRange(1.0, 100.0); spatialRadiusSpinBox->setValue(20.0);

    colorRadiusLabel = new QLabel("Color Radius");
    colorRadiusSpinBox = new QDoubleSpinBox(); colorRadiusSpinBox->setRange(1.0, 150.0); colorRadiusSpinBox->setValue(30.0);

    seedXLabel = new QLabel("Seed X");
    seedXSpinBox = new QSpinBox(); seedXSpinBox->setRange(0, 5000);

    seedYLabel = new QLabel("Seed Y");
    seedYSpinBox = new QSpinBox(); seedYSpinBox->setRange(0, 5000);

    QHBoxLayout *paramsLayout = new QHBoxLayout();
    paramsLayout->addLayout(createParamLayout(kLabel, kSpinBox)); // Used for both K and Spectral Modes
    paramsLayout->addLayout(createParamLayout(thresholdLabel, thresholdSpinBox));
    paramsLayout->addLayout(createParamLayout(windowSizeLabel, windowSizeSpinBox));
    paramsLayout->addLayout(createParamLayout(cValueLabel, cValueSpinBox));
    paramsLayout->addLayout(createParamLayout(spatialRadiusLabel, spatialRadiusSpinBox));
    paramsLayout->addLayout(createParamLayout(colorRadiusLabel, colorRadiusSpinBox));
    paramsLayout->addLayout(createParamLayout(seedXLabel, seedXSpinBox));
    paramsLayout->addLayout(createParamLayout(seedYLabel, seedYSpinBox));

    // -- Center: Algorithm Toggle Buttons --
    methodGroup = new QButtonGroup(this);
    QGridLayout *methodsLayout = new QGridLayout();
    methodsLayout->setHorizontalSpacing(10);
    methodsLayout->setVerticalSpacing(10);

    // Added Spectral and Local to the list
    QStringList methods = {"Otsu", "Optimal", "Spectral", "Local", "K-Means", "Region Growing", "Mean Shift", "Agglomerative"};
    for (int i = 0; i < methods.size(); ++i) {
        QPushButton *btn = new QPushButton(methods[i]);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        if (i == 0) btn->setChecked(true);
        methodGroup->addButton(btn, i);
        methodsLayout->addWidget(btn, i / 4, i % 4); // 2 rows, 4 columns
    }
    connect(methodGroup, &QButtonGroup::idClicked, this, &MainWindow::onMethodChanged);

    // -- Right: Status and Apply Button --
    QVBoxLayout *rightLayout = new QVBoxLayout();
    statusLabel = new QLabel("<b>Status:</b> Waiting for image...");
    timeLabel = new QLabel("<b>Matching time:</b> -- ms");
    processButton = new QPushButton("Process Image");
    processButton->setObjectName("applyButton");
    processButton->setEnabled(false);
    processButton->setCursor(Qt::PointingHandCursor);

    rightLayout->addWidget(statusLabel);
    rightLayout->addWidget(timeLabel);
    rightLayout->addWidget(processButton);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Assemble Top Layout
    topLayout->addLayout(paramsLayout);
    topLayout->addStretch();
    topLayout->addLayout(methodsLayout);
    topLayout->addStretch();
    topLayout->addLayout(rightLayout);
    controlsBox->setLayout(topLayout);

    // --- 2. BOTTOM SECTION (IMAGES) ---
    // Left Image Area
    QLabel *img1Title = new QLabel("Image 1 — double-click to upload");
    img1Title->setObjectName("headerLabel");
    originalImageLabel = new QLabel("Double-click here\nto load image");
    originalImageLabel->setObjectName("imagePanel");
    originalImageLabel->setAlignment(Qt::AlignCenter);
    originalImageLabel->installEventFilter(this); // Enable double-click detection

    QVBoxLayout *leftImgLayout = new QVBoxLayout();
    leftImgLayout->addWidget(img1Title);
    leftImgLayout->addWidget(originalImageLabel, 1);

    // Right Image Area
    QLabel *img2Title = new QLabel("Resulting Output");
    img2Title->setObjectName("headerLabel");
    processedImageLabel = new QLabel("");
    processedImageLabel->setObjectName("imagePanel");
    processedImageLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *rightImgLayout = new QVBoxLayout();
    rightImgLayout->addWidget(img2Title);
    rightImgLayout->addWidget(processedImageLabel, 1);

    QHBoxLayout *imageSpaceLayout = new QHBoxLayout();
    imageSpaceLayout->addLayout(leftImgLayout, 1);
    imageSpaceLayout->addLayout(rightImgLayout, 1);
    imageSpaceLayout->setSpacing(20);

    // --- 3. MAIN LAYOUT ---
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(controlsBox);
    mainLayout->addLayout(imageSpaceLayout, 1);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    connect(processButton, &QPushButton::clicked, this, &MainWindow::processImage);
}

void MainWindow::applyStyleSheet() {
    QString style = R"(
        QMainWindow { background-color: #f0f4f8; }

        /* Group Box Area */
        QGroupBox {
            background-color: #ffffff;
            border: 2px solid #b0bec5;
            border-radius: 8px;
            margin-top: 15px;
            padding-top: 15px;
            font-size: 13px;
            font-weight: bold;
            color: #37474f;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 10px;
            color: #263238;
        }

        /* Labels above the input fields */
        QLabel#paramLabel {
            color: #0277bd; /* Deep Blue */
            font-size: 12px;
            font-weight: bold;
        }

        /* Input Fields (The Number Boxes) */
        QSpinBox, QDoubleSpinBox {
            border: 2px solid #4fc3f7;
            border-radius: 5px;
            padding: 4px;
            background-color: #ffffff;
            color: #e91e63; /* Vivid Pink/Red for the numbers */
            font-weight: bold;
            font-size: 14px;
            min-width: 65px;
        }

        /* The tiny Up/Down Arrow Buttons inside the fields */
        QSpinBox::up-button, QDoubleSpinBox::up-button {
            background-color: #81d4fa; /* Colorful Light Blue */
            border-left: 1px solid #4fc3f7;
            border-bottom: 1px solid #4fc3f7;
            border-top-right-radius: 3px;
            width: 22px;
        }
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background-color: #81d4fa; /* Colorful Light Blue */
            border-left: 1px solid #4fc3f7;
            border-bottom-right-radius: 3px;
            width: 22px;
        }
        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: #29b6f6; /* Darker blue on hover */
        }

        /* Algorithm Toggle Buttons */
        QPushButton {
            background-color: #e3f2fd;
            color: #1565c0; /* Dark blue text */
            border: 2px solid #90caf9;
            border-radius: 15px;
            padding: 6px 15px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover { background-color: #bbdefb; border: 2px solid #64b5f6; }
        QPushButton:checked {
            background-color: #ff9800; /* Vivid Orange for the active button */
            color: white;
            border: 2px solid #f57c00;
        }

        /* Main Process Image Button */
        QPushButton#applyButton {
            background-color: #673ab7; /* Deep Purple */
            color: white;
            font-size: 14px;
            font-weight: bold;
            padding: 8px 20px;
            border-radius: 6px;
            border: none;
        }
        QPushButton#applyButton:hover { background-color: #512da8; }
        QPushButton#applyButton:disabled { background-color: #cfd8dc; color: #90a4ae; }

        /* Image Display Panels */
        QLabel#headerLabel { color: #34495e; font-weight: bold; font-size: 13px; }
        QLabel#imagePanel {
            border: 3px dashed #90a4ae;
            border-radius: 12px;
            background-color: #eceff1;
            color: #607d8b;
            font-size: 14px;
            font-weight: bold;
        }
    )";
    this->setStyleSheet(style);
}

void MainWindow::onMethodChanged(int id) {
    currentMethodId = id;

    // Hide all parameters
    kLabel->hide(); kSpinBox->hide();
    thresholdLabel->hide(); thresholdSpinBox->hide();
    windowSizeLabel->hide(); windowSizeSpinBox->hide();
    cValueLabel->hide(); cValueSpinBox->hide();
    spatialRadiusLabel->hide(); spatialRadiusSpinBox->hide();
    colorRadiusLabel->hide(); colorRadiusSpinBox->hide();
    seedXLabel->hide(); seedXSpinBox->hide();
    seedYLabel->hide(); seedYSpinBox->hide();

    // Show only the ones needed for this specific algorithm
    if (id == 2) { // Spectral Thresholding
        kLabel->setText("Modes");
        kLabel->show(); kSpinBox->show();
    } else if (id == 3) { // Local Thresholding
        windowSizeLabel->show(); windowSizeSpinBox->show();
        cValueLabel->show(); cValueSpinBox->show();
    } else if (id == 4 || id == 7) { // K-Means, Agglomerative
        kLabel->setText("Clusters (k)");
        kLabel->show(); kSpinBox->show();
    } else if (id == 5) { // Region Growing
        thresholdLabel->show(); thresholdSpinBox->show();
        seedXLabel->show(); seedXSpinBox->show();
        seedYLabel->show(); seedYSpinBox->show();
    } else if (id == 6) { // Mean Shift
        spatialRadiusLabel->show(); spatialRadiusSpinBox->show();
        colorRadiusLabel->show(); colorRadiusSpinBox->show();
    }
}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.pgm)");
    if (fileName.isEmpty()) return;

    if (loadedQImage.load(fileName)) {
        isImageLoaded = true;
        processButton->setEnabled(true);
        statusLabel->setText("<b>Status:</b> <span style='color:#27ae60;'>Image Loaded</span>");

        seedXSpinBox->setMaximum(loadedQImage.width() - 1);
        seedYSpinBox->setMaximum(loadedQImage.height() - 1);
        seedXSpinBox->setValue(loadedQImage.width() / 2);
        seedYSpinBox->setValue(loadedQImage.height() / 2);

        QPixmap pixmap = QPixmap::fromImage(loadedQImage).scaled(500, 500, Qt::KeepAspectRatio);
        originalImageLabel->setPixmap(pixmap);
        processedImageLabel->clear();
    } else {
        QMessageBox::critical(this, "Error", "Failed to load image.");
    }
}

void MainWindow::processImage() {
    if (!isImageLoaded) return;

    statusLabel->setText("<b>Status:</b> <span style='color:#e67e22;'>Processing...</span>");
    timeLabel->setText("<b>Matching time:</b> Calculating...");
    QCoreApplication::processEvents(); // Force UI update

    QElapsedTimer timer;
    timer.start(); // Start stopwatch

    // --- GRANTSCALE THRESHOLDING ALGORITHMS ---
    if (currentMethodId >= 0 && currentMethodId <= 3) {
        GrayImage grayInput = qImageToGrayImage(loadedQImage);
        GrayImage result;

        if (currentMethodId == 0) {
            result = Thresholding::applyOtsuThresholding(grayInput);
        } else if (currentMethodId == 1) {
            result = Thresholding::applyOptimalThresholding(grayInput);
        } else if (currentMethodId == 2) {
            result = Thresholding::applySpectralThresholding(grayInput, kSpinBox->value());
        } else if (currentMethodId == 3) {
            result = Thresholding::applyLocalThresholding(grayInput, windowSizeSpinBox->value(), cValueSpinBox->value());
        }
        processedImageLabel->setPixmap(grayImageToQPixmap(result));

    // --- REGION GROWING (Operates on Grayscale usually) ---
    } else if (currentMethodId == 5) {
        GrayImage grayInput = qImageToGrayImage(loadedQImage);
        GrayImage result = Segmentation::applyRegionGrowing(grayInput, seedXSpinBox->value(), seedYSpinBox->value(), thresholdSpinBox->value());
        processedImageLabel->setPixmap(grayImageToQPixmap(result));

    // --- COLOR SEGMENTATION ALGORITHMS ---
    } else {
        // Prevent freezing on heavy algorithms
        QImage safeQImage = loadedQImage;
        if (currentMethodId == 6 && safeQImage.width() > 100) {
            safeQImage = safeQImage.scaledToWidth(100, Qt::SmoothTransformation);
        }

        ColorImage colorInput = qImageToColorImage(safeQImage);
        ColorImage result;

        if (currentMethodId == 4) {
            result = Segmentation::applyKMeans(colorInput, kSpinBox->value(), 10);
        } else if (currentMethodId == 6) {
            result = Segmentation::applyMeanShift(colorInput, spatialRadiusSpinBox->value(), colorRadiusSpinBox->value(), 5);
        } else if (currentMethodId == 7) {
            result = Segmentation::applyAgglomerative(colorInput, kSpinBox->value());
        }
        
        processedImageLabel->setPixmap(colorImageToQPixmap(result));
    }

    qint64 elapsed = timer.elapsed(); // Stop stopwatch
    
    statusLabel->setText("<b>Status:</b> <span style='color:#27ae60;'>Complete!</span>");
    timeLabel->setText(QString("<b>Matching time:</b> %1 ms").arg(elapsed));
}

// --- CONVERSION HELPERS ---

GrayImage MainWindow::qImageToGrayImage(const QImage& qimg) {
    QImage grayQImg = qimg.convertToFormat(QImage::Format_Grayscale8);
    GrayImage img = {grayQImg.width(), grayQImg.height(), std::vector<uint8_t>(grayQImg.width() * grayQImg.height())};
    for (int y = 0; y < img.height; ++y) {
        const uint8_t* scanline = grayQImg.scanLine(y);
        std::copy(scanline, scanline + img.width, img.data.begin() + y * img.width);
    }
    return img;
}

ColorImage MainWindow::qImageToColorImage(const QImage& qimg) {
    QImage rgbQImg = qimg.convertToFormat(QImage::Format_RGB888);
    ColorImage img = {rgbQImg.width(), rgbQImg.height(), std::vector<RGB>(rgbQImg.width() * rgbQImg.height())};
    for (int y = 0; y < img.height; ++y) {
        const uint8_t* scanline = rgbQImg.scanLine(y);
        for (int x = 0; x < img.width; ++x) {
            img.data[y * img.width + x] = {scanline[x * 3], scanline[x * 3 + 1], scanline[x * 3 + 2]};
        }
    }
    return img;
}

QPixmap MainWindow::grayImageToQPixmap(const GrayImage& img) {
    QImage qimg(img.data.data(), img.width, img.height, img.width, QImage::Format_Grayscale8);
    return QPixmap::fromImage(qimg.copy()).scaled(500, 500, Qt::KeepAspectRatio);
}

QPixmap MainWindow::colorImageToQPixmap(const ColorImage& img) {
    QImage qimg((const uchar*)img.data.data(), img.width, img.height, img.width * 3, QImage::Format_RGB888);
    return QPixmap::fromImage(qimg.copy()).scaled(500, 500, Qt::KeepAspectRatio);
}