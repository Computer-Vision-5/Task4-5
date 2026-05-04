#include "FaceRecognizer.h"
#include "facedetectcnn.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QColor>
#include <QDataStream>

#include <QFile>
#include <QDebug>

#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>
#include <memory>
#include <random>

#define DETECT_BUFFER_SIZE 0x20000

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

FaceRecognizer::FaceRecognizer() = default;

// ─────────────────────────────────────────────────────────────────────────────
//  Accessors
// ─────────────────────────────────────────────────────────────────────────────

int FaceRecognizer::numSubjects() const
{
    if (m_labels.empty()) return 0;
    // Use an unordered_set for O(N) instead of O(N log N) sort+unique
    std::unordered_set<std::string> unique(m_labels.begin(), m_labels.end());
    return static_cast<int>(unique.size());
}

// ─────────────────────────────────────────────────────────────────────────────
//  A. Preprocessing helpers
// ─────────────────────────────────────────────────────────────────────────────

QString FaceRecognizer::extractPersonId(const QString &filename) const
{
    QFileInfo fileInfo(filename);
    QString baseName = fileInfo.baseName();
    return baseName.section('_', 0, 0);
}

// FIX: Return both the cropped image and the bounding rect so that
//      recognize() does not need to run the CNN detector a second time.
FaceRecognizer::DetectResult FaceRecognizer::detectAndCrop(const QImage &img) const
{
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);
    QImage bgr = rgb.rgbSwapped();

    int width  = bgr.width();
    int height = bgr.height();
    int step   = bgr.bytesPerLine();

    // RAII buffer — never leaks, even on early return
    auto pBuffer = std::make_unique<unsigned char[]>(DETECT_BUFFER_SIZE);

    int *pResults = facedetect_cnn(pBuffer.get(),
                                   const_cast<unsigned char*>(bgr.bits()),
                                   width, height, step);

    int faceCount = (pResults ? *pResults : 0);
    if (faceCount == 0)
        return {};   // crop.isNull() == true, rect is zero

    // Pick the largest detected face
    int bestIdx  = 0;
    int bestArea = 0;
    for (int i = 0; i < faceCount; ++i) {
        // FIX: offset pointer by 142*i so we examine the correct face entry
        short *p = ((short*)(pResults + 1)) + 142 * i;
        int w = p[3], h = p[4];
        if (w * h > bestArea) {
            bestArea = w * h;
            bestIdx  = i;
        }
    }

    // FIX: use bestIdx, not 0, when reading the final bounding box
    short *p = ((short*)(pResults + 1)) + 142 * bestIdx;
    int x = p[1], y = p[2], w = p[3], h = p[4];

    // Clamp to image bounds
    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(w, width  - x);
    h = std::min(h, height - y);

    DetectResult res;
    res.crop = img.copy(x, y, w, h);
    res.rect = QRect(x, y, w, h);
    return res;
}

std::vector<float> FaceRecognizer::preprocessFace(const QImage &crop) const
{
    // Sanity check: FACE_DIM must equal FACE_W * FACE_H
    static_assert(FACE_DIM == FACE_W * FACE_H,
                  "FACE_DIM must equal FACE_W * FACE_H");

    QImage resized = crop
        .scaled(FACE_W, FACE_H, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .convertToFormat(QImage::Format_Grayscale8);

    std::vector<float> vec(FACE_DIM);
    for (int row = 0; row < FACE_H; ++row) {
        const uchar *line = resized.constScanLine(row);
        for (int col = 0; col < FACE_W; ++col)
            vec[row * FACE_W + col] = static_cast<float>(line[col]);
    }
    return vec;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PCA internals
// ─────────────────────────────────────────────────────────────────────────────

float FaceRecognizer::euclidean(const std::vector<float> &a, const std::vector<float> &b)
{
    float sum = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum);
}

std::vector<float> FaceRecognizer::project(const std::vector<float> &face) const
{
    std::vector<float> centered(FACE_DIM);
    for (int i = 0; i < FACE_DIM; ++i)
        centered[i] = face[i] - m_mean[i];

    std::vector<float> coeff(m_numComponents, 0.f);
    for (int k = 0; k < m_numComponents; ++k) {
        float dot = 0.f;
        const auto &ef = m_eigenfaces[k];
        for (int i = 0; i < FACE_DIM; ++i)
            dot += centered[i] * ef[i];
        coeff[k] = dot;
    }
    return coeff;
}

void FaceRecognizer::computePCA()
{
    const int N   = static_cast<int>(m_trainingFaces.size());
    const int dim = FACE_DIM;

    // ── 1. Compute mean face ─────────────────────────────────────────────────
    m_mean.assign(dim, 0.f);
    for (const auto &f : m_trainingFaces)
        for (int i = 0; i < dim; ++i)
            m_mean[i] += f[i];
    for (int i = 0; i < dim; ++i)
        m_mean[i] /= static_cast<float>(N);

    // ── 2. Build centered matrix A (N × D) ───────────────────────────────────
    std::vector<std::vector<float>> A(N, std::vector<float>(dim));
    for (int n = 0; n < N; ++n)
        for (int i = 0; i < dim; ++i)
            A[n][i] = m_trainingFaces[n][i] - m_mean[i];

    // ── 3. Turk-Pentland small matrix L = A·Aᵀ (N×N) ────────────────────────
    std::vector<std::vector<float>> L(N, std::vector<float>(N, 0.f));
    for (int i = 0; i < N; ++i)
        for (int j = i; j < N; ++j) {
            float dot = 0.f;
            for (int k = 0; k < dim; ++k)
                dot += A[i][k] * A[j][k];
            L[i][j] = L[j][i] = dot;
        }

    const int maxComp = std::min(N - 1, 50);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);

    std::vector<std::vector<float>> Lwork    = L;
    std::vector<std::vector<float>> smallVecs;
    std::vector<float>              rawEigenvalues;

    for (int k = 0; k < maxComp; ++k) {

        std::vector<float> v(N);
        for (int i = 0; i < N; ++i) v[i] = dist(rng);

        float initNorm = 0.f;
        for (float x : v) initNorm += x * x;
        initNorm = std::sqrt(initNorm);
        if (initNorm > 1e-10f)
            for (float &x : v) x /= initNorm;

        // FIX: Track convergence and warn if power iteration fails to settle.
        //      A failed eigenvector is replaced with a zero vector so it
        //      contributes nothing to projections rather than injecting noise.
        bool converged = false;
        std::vector<float> vPrev(N, 0.f);

        for (int iter = 0; iter < 300; ++iter) {
            std::vector<float> w(N, 0.f);
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                    w[i] += Lwork[i][j] * v[j];

            float norm = 0.f;
            for (float x : w) norm += x * x;
            norm = std::sqrt(norm);

            if (norm < 1e-10f) break;   // true zero eigenvalue — stop cleanly

            for (int i = 0; i < N; ++i) v[i] = w[i] / norm;

            // Check convergence: |v - vPrev| < tolerance
            float diff = 0.f;
            for (int i = 0; i < N; ++i) {
                float d = v[i] - vPrev[i];
                diff += d * d;
            }
            if (std::sqrt(diff) < 1e-6f) {
                converged = true;
                break;
            }
            vPrev = v;
        }

        if (!converged) {
            qWarning() << QString("PCA: eigenvector %1 did not converge — "
                                  "zeroing to avoid noise injection.").arg(k);
            std::fill(v.begin(), v.end(), 0.f);
        }

        float lambda = 0.f;
        for (int i = 0; i < N; ++i) {
            float Lv_i = 0.f;
            for (int j = 0; j < N; ++j)
                Lv_i += Lwork[i][j] * v[j];
            lambda += v[i] * Lv_i;
        }

        smallVecs.push_back(v);
        rawEigenvalues.push_back(lambda);

        // Deflate
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                Lwork[i][j] -= lambda * v[i] * v[j];
    }

    // ── 5. Select K via cumulative variance threshold ─────────────────────────
    float totalVar = 0.f;
    for (float ev : rawEigenvalues) totalVar += std::max(0.f, ev);

    // When varianceThreshold == 0.0 we intentionally keep all components.
    int numComp = maxComp;
    if (m_varianceThreshold > 0.0 && totalVar > 1e-10f) {
        float cumVar = 0.f;
        numComp = 0;
        for (int k = 0; k < maxComp; ++k) {
            cumVar  += std::max(0.f, rawEigenvalues[k]);
            numComp  = k + 1;
            if (cumVar / totalVar >= static_cast<float>(m_varianceThreshold))
                break;
        }
    }

    m_numComponents = numComp;
    qDebug() << QString("PCA: retained %1 / %2 components "
                        "covering >= %3% variance")
                    .arg(numComp).arg(maxComp)
                    .arg(static_cast<int>(m_varianceThreshold * 100));

    // ── 6. Map small eigenvectors back to image space (eigenfaces) ────────────
    m_eigenfaces.assign(numComp, std::vector<float>(dim, 0.f));
    m_eigenvalues.resize(numComp);

    for (int k = 0; k < numComp; ++k) {
        m_eigenvalues[k] = rawEigenvalues[k];

        for (int n = 0; n < N; ++n) {
            float c = smallVecs[k][n];
            for (int i = 0; i < dim; ++i)
                m_eigenfaces[k][i] += c * A[n][i];
        }
        float norm = 0.f;
        for (float x : m_eigenfaces[k]) norm += x * x;
        norm = std::sqrt(norm);
        if (norm > 1e-10f)
            for (float &x : m_eigenfaces[k]) x /= norm;
    }

    // ── 7. Project all training faces into face-space ─────────────────────────
    m_projections.resize(N);
    for (int n = 0; n < N; ++n)
        m_projections[n] = project(m_trainingFaces[n]);

    // ── 8. Free training data — no longer needed ─────────────────────────────
    { std::vector<std::vector<float>> empty; m_trainingFaces.swap(empty); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  B.1  Training
// ─────────────────────────────────────────────────────────────────────────────

int FaceRecognizer::trainFromFolder(const QString &trainPath,
                                    double varianceThreshold)
{
    m_varianceThreshold = varianceThreshold;
    m_trained           = false;
    m_trainingFaces.clear();
    m_labels.clear();
    m_mean.clear();
    m_eigenfaces.clear();
    m_eigenvalues.clear();
    m_projections.clear();

    QDir dir(trainPath);
    if (!dir.exists()) {
        qWarning() << "FaceRecognizer: Training folder not found:" << trainPath;
        return 0;
    }

    QStringList filters = {"*.jpg","*.jpeg","*.png","*.bmp","*.tif","*.tiff"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    int loaded = 0;
    for (const QFileInfo &fi : files) {
        QString personId = extractPersonId(fi.fileName());

        QImage img(fi.absoluteFilePath());
        if (img.isNull()) continue;

        // Try to crop to the detected face; fall back to the whole image
        DetectResult det = detectAndCrop(img);
        QImage crop = det.crop.isNull() ? img : det.crop;

        std::vector<float> vec = preprocessFace(crop);
        m_trainingFaces.push_back(vec);
        m_labels.push_back(personId.toStdString());
        ++loaded;

        if (loaded % 20 == 0)
            qDebug() << "Loaded" << loaded << "training faces...";
    }

    if (loaded < 2) {
        qWarning() << "FaceRecognizer: Need at least 2 training faces.";
        return loaded;
    }

    qDebug() << "Running PCA on" << loaded
             << "faces from" << numSubjects() << "subjects...";
    computePCA();

    m_trained = true;
    qDebug() << "Training complete. Components used:" << m_numComponents;
    return loaded;
}

bool FaceRecognizer::trainAndSaveModel(const QString &trainDataDir,
                                       const QString &modelPath,
                                       double varianceThreshold)
{
    int loaded = trainFromFolder(trainDataDir, varianceThreshold);
    if (loaded < 2 || !m_trained) return false;
    return saveModel(modelPath);
}

// ─────────────────────────────────────────────────────────────────────────────
//  B.2  Recognition
// ─────────────────────────────────────────────────────────────────────────────

QString FaceRecognizer::recognize(QImage &image, double &outConfidence)
{
    outConfidence = 0.0;
    if (!m_trained) return {};

    // ── 1. Detect and crop face — ONE detector call, result reused for drawing
    DetectResult det = detectAndCrop(image);
    if (det.crop.isNull()) return {};

    // ── 2. Project into face-space and find nearest neighbour ────────────────
    std::vector<float> vec   = preprocessFace(det.crop);
    std::vector<float> coeff = project(vec);

    float minDist = std::numeric_limits<float>::max();
    int   bestIdx = -1;

    for (int n = 0; n < static_cast<int>(m_projections.size()); ++n) {
        float d = euclidean(coeff, m_projections[n]);
        if (d < minDist) {
            minDist = d;
            bestIdx = n;
        }
    }

    if (bestIdx < 0) return {};

    QString label = QString::fromStdString(m_labels[bestIdx]);

    // ── 3. Compute confidence using nearest inter-class distance ─────────────
    //
    //  Confidence is derived from the ratio:  minDist / secDist
    //
    //  Where secDist is the distance to the closest sample from a DIFFERENT
    //  person.  A ratio near 0 means the query is much closer to the matched
    //  person than to anyone else → high confidence.  A ratio near 1 means the
    //  margin is negligible → low confidence.
    //
    //  Mapping:
    //    ratio <= CONF_RATIO_PERFECT  → 100 % confidence
    //    ratio >= 1.0                 →   0 % confidence
    //    linear interpolation between the two endpoints
    //
    //  CONF_RATIO_PERFECT = 0.7 is a reasonable starting point for PCA/eigenfaces
    //  on standard face datasets; tune it via cross-validation if needed.
    //
    constexpr double CONF_RATIO_PERFECT = 0.7;   // ratio at which we call it 100%

    float secDist = std::numeric_limits<float>::max();
    for (int n = 0; n < static_cast<int>(m_projections.size()); ++n) {
        if (m_labels[n] != m_labels[bestIdx]) {
            float d = euclidean(coeff, m_projections[n]);
            if (d < secDist)
                secDist = d;
        }
    }

    if (secDist == std::numeric_limits<float>::max()) {
        // Only one subject in the dataset — confidence is trivially 1.
        outConfidence = 1.0;
    } else {
        // ratio in [0, ∞); clamp result to [0, 1].
        double ratio = static_cast<double>(minDist) /
                       static_cast<double>(secDist);
        // Linear map: CONF_RATIO_PERFECT → 1.0,  1.0 → 0.0
        outConfidence = (1.0 - ratio) / (1.0 - CONF_RATIO_PERFECT);
        outConfidence = std::max(0.0, std::min(1.0, outConfidence));
    }

    // ── 4. Draw the bounding box on the caller's image ───────────────────────
    //   FIX: reuse det.rect — no second CNN call needed.
    if (!det.rect.isNull()) {
        const QRect &r = det.rect;
        QPainter painter(&image);
        painter.setPen(QPen(Qt::blue, 2));
        painter.drawRect(r);

        QRect textRect(r.x(), r.y() + r.height(), 130, 22);
        painter.fillRect(textRect, QColor(0, 0, 255, 160));
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 10, QFont::Bold));
        painter.drawText(r.x() + 3, r.y() + r.height() + 16,
                         QString("%1 (%2%)")
                             .arg(label)
                             .arg(static_cast<int>(outConfidence * 100)));
    }

    return label;
}

QString FaceRecognizer::recognize(QImage &image)
{
    double dummy;
    return recognize(image, dummy);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Persistence
// ─────────────────────────────────────────────────────────────────────────────

bool FaceRecognizer::saveModel(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QDataStream ds(&f);

    ds << m_trained << m_numComponents << (qint32)FACE_W << (qint32)FACE_H;

    ds << (qint32)m_mean.size();
    for (float v : m_mean) ds << v;

    ds << (qint32)m_eigenfaces.size();
    for (const auto &ef : m_eigenfaces) {
        ds << (qint32)ef.size();
        for (float v : ef) ds << v;
    }

    ds << (qint32)m_eigenvalues.size();
    for (float v : m_eigenvalues) ds << v;

    ds << (qint32)m_projections.size();
    for (int n = 0; n < (int)m_projections.size(); ++n) {
        ds << (qint32)m_projections[n].size();
        for (float v : m_projections[n]) ds << v;
        ds << QString::fromStdString(m_labels[n]);
    }

    return true;
}

bool FaceRecognizer::loadModel(const QString &filePath)
{
    m_trained = false;
    m_mean.clear();
    m_eigenfaces.clear();
    m_eigenvalues.clear();
    m_projections.clear();
    m_labels.clear();

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QDataStream ds(&f);

    bool trained; int numComp; qint32 fw, fh;
    ds >> trained >> numComp >> fw >> fh;
    if (fw != FACE_W || fh != FACE_H) {
        qWarning() << "FaceRecognizer::loadModel: dimension mismatch in saved model.";
        return false;
    }
    m_numComponents = numComp;

    qint32 sz;
    ds >> sz; m_mean.resize(sz);
    for (float &v : m_mean) ds >> v;

    ds >> sz; m_eigenfaces.resize(sz);
    for (auto &ef : m_eigenfaces) {
        qint32 esz; ds >> esz; ef.resize(esz);
        for (float &v : ef) ds >> v;
    }

    if (!ds.atEnd()) {
        ds >> sz; m_eigenvalues.resize(sz);
        for (float &v : m_eigenvalues) ds >> v;
    }

    ds >> sz;
    m_projections.resize(sz);
    m_labels.resize(sz);
    for (int n = 0; n < sz; ++n) {
        qint32 psz; ds >> psz; m_projections[n].resize(psz);
        for (float &v : m_projections[n]) ds >> v;
        QString lbl; ds >> lbl;
        m_labels[n] = lbl.toStdString();
    }

    if (ds.status() != QDataStream::Ok) {
        qWarning() << "FaceRecognizer::loadModel: stream error while reading model.";
        m_trained = false;
        return false;
    }

    m_trained = trained;
    return true;
}