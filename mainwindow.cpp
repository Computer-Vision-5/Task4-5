#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPainter>
#include <QDir>
#include "facedetectcnn.h"

// Define a macro for facedetect bounding box processing
#define DETECT_BUFFER_SIZE 0x20000

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    // Set the main window background to white
    centralWidget->setStyleSheet("QWidget#centralWidget { background-color: #ffffff; }");
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(25);

    uploadButton = new QPushButton("Upload Image", this);
    uploadButton->setCursor(Qt::PointingHandCursor);
    
    // Modern button styling
    QString buttonStyle = 
        "QPushButton {"
        "  background-color: #0d6efd;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "  font-size: 15px;"
        "  padding: 12px 24px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b5ed7;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0a58ca;"
        "}";
    uploadButton->setStyleSheet(buttonStyle);
    // Prevent button from stretching horizontally
    uploadButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Center the button in a horizontal layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(uploadButton);
    buttonLayout->addStretch();

    imageLabel = new QLabel("No image loaded", this);
    imageLabel->setAlignment(Qt::AlignCenter);
    
    // Clean, light placeholder styling
    QString labelStyle = 
        "QLabel {"
        "  background-color: #f8f9fa;"
        "  color: #6c757d;"
        "  font-size: 18px;"
        "  border: 2px dashed #dee2e6;"
        "  border-radius: 12px;"
        "}";
    imageLabel->setStyleSheet(labelStyle);
    
    // Adjust window and image sizes
    setMinimumSize(900, 700);
    imageLabel->setMinimumSize(800, 500);

    layout->addLayout(buttonLayout);
    layout->addWidget(imageLabel);
    layout->setStretchFactor(imageLabel, 1);

    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::uploadImage);
}

MainWindow::~MainWindow()
{
}

void MainWindow::uploadImage()
{
    // Path to the "whole images" directory in the project root
    QString initialDir = QDir::currentPath() + "/whole images";
    
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), initialDir, tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));

    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (image.isNull()) {
            imageLabel->setText("Failed to load image.");
            return;
        }

        // Process the image for face detection
        detectFaces(image);

        // Display the processed image
        // Scale it to fit the label while keeping aspect ratio
        QPixmap pixmap = QPixmap::fromImage(image);
        imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::detectFaces(QImage &image)
{
    // Convert image to a format suitable for libfacedetection
    // libfacedetection requires an unsigned char array of BGR values.
    // QImage::Format_RGB888 is RGB, so we need to convert it and swap R and B.
    
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    
    // Convert RGB to BGR for libfacedetection
    // QImage::rgbSwapped() swaps Red and Blue channels
    QImage bgrImage = rgbImage.rgbSwapped();

    int width = bgrImage.width();
    int height = bgrImage.height();
    int step = bgrImage.bytesPerLine();
    
    unsigned char *pBuffer = new unsigned char[DETECT_BUFFER_SIZE];
    
    // facedetect_cnn parameters:
    // pBuffer: buffer memory for storing the results
    // result_buffer: memory buffer size
    // bgrImage.bits(): BGR image data
    // width: image width
    // height: image height
    // step: bytes per line
    
    int * pResults = facedetect_cnn(pBuffer, bgrImage.bits(), width, height, step);

    QPainter painter(&image);
    painter.setPen(QPen(Qt::green, 3));
    
    int faceCount = (pResults ? *pResults : 0);
    
    for (int i = 0; i < faceCount; i++) {
        // facedetect_cnn output format for each face:
        // ptr[0]: confidence
        // ptr[1]: x
        // ptr[2]: y
        // ptr[3]: w
        // ptr[4]: h
        // The rest are facial landmarks.
        // The size of each face record is 142 shorts usually. 
        
        short * p = ((short*)(pResults + 1)) + 142 * i;
        int confidence = p[0];
        int x = p[1];
        int y = p[2];
        int w = p[3];
        int h = p[4];

        // Draw rectangle around the face
        painter.drawRect(x, y, w, h);
        
        // Draw confidence
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        
        // Add a nice background for the text
        QRect textRect(x, y - 20, 100, 20);
        painter.fillRect(textRect, QColor(0, 255, 0, 128));
        
        painter.setPen(Qt::black);
        painter.drawText(x + 2, y - 5, QString("Face: %1%").arg(confidence));
        painter.setPen(QPen(Qt::green, 3));
    }

    delete[] pBuffer;
}
