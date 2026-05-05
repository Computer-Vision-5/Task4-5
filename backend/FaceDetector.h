#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <QImage>

// FaceDetector provides detection-only annotation: it draws a green bounding
// box and CNN confidence score around every face found in an image, without
// performing any identity recognition.
//
// For identity recognition (blue box + name label), use FaceRecognizer::recognize().

class FaceDetector
{
public:
    // Detects all faces in `image` and draws a green bounding box with a
    // "Face: X%" confidence label over each one.  The image is modified
    // in-place.  Does nothing if no faces are detected.
    static void processImage(QImage &image);
};

#endif // FACEDETECTOR_H