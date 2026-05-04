#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPainter>
#include <QDir>
#include "../backend/FaceDetector.h"


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
        FaceDetector::processImage(image);

        // Display the processed image
        // Scale it to fit the label while keeping aspect ratio
        QPixmap pixmap = QPixmap::fromImage(image);
        imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
