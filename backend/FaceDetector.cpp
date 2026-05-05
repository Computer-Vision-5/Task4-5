#include "FaceDetector.h"
#include "facedetect_common.h"   // FACEDETECT_BUFFER_SIZE, FACEDETECT_RESULT_STRIDE
#include "facedetectcnn.h"

#include <QPainter>
#include <QFont>
#include <QColor>

#include <memory>                // std::make_unique
#include <algorithm>             // std::max / std::min

void FaceDetector::processImage(QImage &image)
{
    if (image.isNull())
        return;

    // libfacedetection requires BGR byte order.
    QImage bgr = image.convertToFormat(QImage::Format_RGB888).rgbSwapped();

    const int width  = bgr.width();
    const int height = bgr.height();
    const int step   = bgr.bytesPerLine();

    // RAII buffer — no raw new/delete, never leaks on early return.
    auto pBuffer = std::make_unique<unsigned char[]>(FACEDETECT_BUFFER_SIZE);

    int *pResults = facedetect_cnn(pBuffer.get(),
                                   const_cast<unsigned char *>(bgr.bits()),
                                   width, height, step);

    const int faceCount = (pResults ? *pResults : 0);
    if (faceCount == 0)
        return;

    QPainter painter(&image);

    for (int i = 0; i < faceCount; ++i) {
        // Result layout per detection (each entry = 16 shorts):
        //   p[0]  = confidence  (0–100)
        //   p[1]  = x,  p[2] = y,  p[3] = w,  p[4] = h
        //   p[5..14] = five facial landmarks (x,y pairs)
        // Stride is 16 shorts (32 bytes) per face.
        // NOTE: older code incorrectly used 142*i here, which jumped 284 bytes
        //       per face and read garbage memory for every face after the first.
        const short *p = reinterpret_cast<const short *>(pResults + 1)
                         + FACEDETECT_RESULT_STRIDE * i;

        const int confidence = p[0];   // 0–100

        // Clamp bounding box to image bounds — CNN can return coords outside
        // the image for faces near edges.
        const int x = std::max(0, static_cast<int>(p[1]));
        const int y = std::max(0, static_cast<int>(p[2]));
        const int w = std::min(static_cast<int>(p[3]), width  - x);
        const int h = std::min(static_cast<int>(p[4]), height - y);

        if (w <= 0 || h <= 0)
            continue;

        // ── Bounding box ────────────────────────────────────────────────────
        painter.setPen(QPen(Qt::green, 3));
        painter.drawRect(x, y, w, h);

        // ── Confidence label ─────────────────────────────────────────────────
        // Place above the box; flip below if the box is at the top of the image.
        const bool labelAbove = (y >= 20);
        const QRect textRect = labelAbove
            ? QRect(x, y - 20, 110, 20)
            : QRect(x, y + h,  110, 20);

        painter.fillRect(textRect, QColor(0, 200, 0, 160));
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.setPen(Qt::black);
        painter.drawText(textRect.adjusted(3, 0, 0, 0),
                         Qt::AlignVCenter,
                         QString("Face: %1%").arg(confidence));

        painter.setPen(QPen(Qt::green, 3));
    }
}