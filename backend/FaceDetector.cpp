#include "FaceDetector.h"
#include <QPainter>
#include "facedetectcnn.h"

// Define a macro for facedetect bounding box processing
#define DETECT_BUFFER_SIZE 0x20000

void FaceDetector::processImage(QImage &image)
{
    // Convert image to a format suitable for libfacedetection
    // libfacedetection requires an unsigned char array of BGR values.
    // QImage::Format_RGB888 is RGB, so we need to convert it and swap R and B.
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
    
    // Convert RGB to BGR for libfacedetection
    QImage bgrImage = rgbImage.rgbSwapped();

    int width = bgrImage.width();
    int height = bgrImage.height();
    int step = bgrImage.bytesPerLine();
    
    unsigned char *pBuffer = new unsigned char[DETECT_BUFFER_SIZE];
    
    int * pResults = facedetect_cnn(pBuffer, bgrImage.bits(), width, height, step);

    QPainter painter(&image);
    painter.setPen(QPen(Qt::green, 3));
    
    int faceCount = (pResults ? *pResults : 0);
    
    for (int i = 0; i < faceCount; i++) {
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
