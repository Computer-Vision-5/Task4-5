#ifndef FACEDETECT_COMMON_H
#define FACEDETECT_COMMON_H

// ── libfacedetection shared constants ────────────────────────────────────────
//
// Buffer size required by facedetect_cnn().  Must be exactly 0x20000 bytes.
// (Some older examples show 0x9000, but 0x20000 is the safe/required value
//  for the version of the library used in this project.)
static constexpr int FACEDETECT_BUFFER_SIZE = 0x20000;

// Stride between consecutive face detection entries in the result array.
//
// facedetect_cnn() returns an int* pointing into the result buffer:
//   pResults[0]          = number of faces detected
//   ((short*)(pResults+1)) + FACEDETECT_RESULT_STRIDE * i
//                        = pointer to face i's data
//
// Each face entry is 16 shorts (32 bytes):
//   p[0]        confidence     (0–100)
//   p[1]        x              bounding box left edge
//   p[2]        y              bounding box top edge
//   p[3]        w              bounding box width
//   p[4]        h              bounding box height
//   p[5..14]    landmarks      five (x,y) pairs for eyes, nose, mouth corners
//
static constexpr int FACEDETECT_RESULT_STRIDE = 16;

#endif // FACEDETECT_COMMON_H