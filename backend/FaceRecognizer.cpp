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

#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>
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
    std::vector<std::string> tmp = m_labels;
    std::sort(tmp.begin(), tmp.end());
    tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());
    return static_cast<int>(tmp.size());
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

QImage FaceRecognizer::detectAndCrop(const QImage &img) const
{
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);
    QImage bgr = rgb.rgbSwapped();

    int width  = bgr.width();
    int height = bgr.height();
    int step   = bgr.bytesPerLine();

    unsigned char *pBuffer = new unsigned char[DETECT_BUFFER_SIZE];
    int *pResults = facedetect_cnn(pBuffer,
                                   const_cast<unsigned char*>(bgr.bits()),
                                   width, height, step);

    int faceCount = (pResults ? *pResults : 0);
    if (faceCount == 0) {
        delete[] pBuffer;
        return QImage();
    }

    // Pick the largest detected face
    int bestIdx  = 0;
    int bestArea = 0;
    for (int i = 0; i < faceCount; ++i) {
        short *p = ((short*)(pResults + 1)) + 142 * i;
        int w = p[3], h = p[4];
        if (w * h > bestArea) {
            bestArea = w * h;
            bestIdx  = i;
        }
    }

    short *p = ((short*)(pResults + 1)) + 142 * bestIdx;
    int x = p[1], y = p[2], w = p[3], h = p[4];

    // Clamp to image bounds
    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(w, width  - x);
    h = std::min(h, height - y);

    QImage crop = img.copy(x, y, w, h);
    delete[] pBuffer;
    return crop;
}

std::vector<float> FaceRecognizer::preprocessFace(const QImage &crop) const
{
    // Resize to fixed size, convert to grayscale, flatten to 1-D float vector
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
    // Subtract mean, then dot with each eigenface
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
    //   Because N << D, the non-zero eigenvalues of AᵀA equal those of L.
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

        // Random initialisation avoids slow convergence for near-equal eigenvalues
        std::vector<float> v(N);
        for (int i = 0; i < N; ++i) v[i] = dist(rng);

        // Normalise initial guess
        float initNorm = 0.f;
        for (float x : v) initNorm += x * x;
        initNorm = std::sqrt(initNorm);
        if (initNorm > 1e-10f)
            for (float &x : v) x /= initNorm;

        // Power iteration (300 steps is sufficient for face datasets)
        for (int iter = 0; iter < 300; ++iter) {
            std::vector<float> w(N, 0.f);
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                    w[i] += Lwork[i][j] * v[j];

            float norm = 0.f;
            for (float x : w) norm += x * x;
            norm = std::sqrt(norm);
            if (norm < 1e-10f) break;
            for (int i = 0; i < N; ++i) v[i] = w[i] / norm;
        }

        // Rayleigh quotient → eigenvalue
        float lambda = 0.f;
        for (int i = 0; i < N; ++i) {
            float Lv_i = 0.f;
            for (int j = 0; j < N; ++j)
                Lv_i += Lwork[i][j] * v[j];
            lambda += v[i] * Lv_i;
        }

        smallVecs.push_back(v);
        rawEigenvalues.push_back(lambda);

        // Deflate: Lwork -= λ · v·vᵀ
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                Lwork[i][j] -= lambda * v[i] * v[j];
    }

    // ── 5. Select K via cumulative variance threshold ─────────────────────────
    float totalVar = 0.f;
    for (float ev : rawEigenvalues) totalVar += std::max(0.f, ev);

    int numComp = maxComp; // fallback: keep all
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
        // Unit-normalise each eigenface
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
        QImage crop = detectAndCrop(img);
        if (crop.isNull())
            crop = img;

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
    computePCA();   // also frees m_trainingFaces

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

    // ── 1. Detect and crop face ──────────────────────────────────────────────
    QImage crop = detectAndCrop(image);
    if (crop.isNull()) return {};

    // ── 2. Project into face-space and find nearest neighbour ────────────────
    std::vector<float> vec   = preprocessFace(crop);
    std::vector<float> coeff = project(vec);

    float minDist = std::numeric_limits<float>::max();
    float secDist = std::numeric_limits<float>::max();
    int   bestIdx = -1;

    for (int n = 0; n < static_cast<int>(m_projections.size()); ++n) {
        float d = euclidean(coeff, m_projections[n]);
        if (d < minDist) {
            secDist = minDist;
            minDist = d;
            bestIdx = n;
        } else if (d < secDist) {
            secDist = d;
        }
    }

    if (bestIdx < 0) return {};

    // Confidence: 0 (same distance as second-nearest) … 1 (much closer)
    outConfidence = (secDist > 1e-6f)
                    ? static_cast<double>(1.0f - minDist / secDist)
                    : 1.0;
    outConfidence = std::max(0.0, std::min(1.0, outConfidence));

    QString label = QString::fromStdString(m_labels[bestIdx]);

    // ── 3. Draw the bounding box on the caller's image ───────────────────────
    //   Re-use the bounding box already found in detectAndCrop() by running
    //   the detector once more on the original image.  This is unavoidable here
    //   because detectAndCrop() returns only the cropped pixels, not the rect.
    {
        unsigned char *pBuffer = new unsigned char[DETECT_BUFFER_SIZE];
        QImage bgr = image.convertToFormat(QImage::Format_RGB888).rgbSwapped();
        int *pResults = facedetect_cnn(pBuffer,
                                       const_cast<unsigned char*>(bgr.bits()),
                                       bgr.width(), bgr.height(),
                                       bgr.bytesPerLine());

        if (pResults && *pResults > 0) {
            short *p = ((short*)(pResults + 1));
            int x = p[1], y = p[2], w = p[3], h = p[4];

            QPainter painter(&image);
            painter.setPen(QPen(Qt::blue, 2));
            painter.drawRect(x, y, w, h);

            QRect textRect(x, y + h, 130, 22);
            painter.fillRect(textRect, QColor(0, 0, 255, 160));
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            painter.drawText(x + 3, y + h + 16,
                             QString("%1 (%2%)")
                                 .arg(label)
                                 .arg(static_cast<int>(outConfidence * 100)));
        }
        delete[] pBuffer;
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

    // Header
    ds << m_trained << m_numComponents << (qint32)FACE_W << (qint32)FACE_H;

    // Mean face (D floats)
    ds << (qint32)m_mean.size();
    for (float v : m_mean) ds << v;

    // Eigenfaces (K × D)
    ds << (qint32)m_eigenfaces.size();
    for (const auto &ef : m_eigenfaces) {
        ds << (qint32)ef.size();
        for (float v : ef) ds << v;
    }

    // Eigenvalues (K) — stored for analysis / future variance-based loading
    ds << (qint32)m_eigenvalues.size();
    for (float v : m_eigenvalues) ds << v;

    // Training projections + labels (N entries)
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
    // Reset to a safe "untrained" state before reading anything,
    // so a partial-read failure never leaves a half-initialised object.
    m_trained = false;
    m_mean.clear();
    m_eigenfaces.clear();
    m_eigenvalues.clear();
    m_projections.clear();
    m_labels.clear();

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QDataStream ds(&f);

    // Header
    bool trained; int numComp; qint32 fw, fh;
    ds >> trained >> numComp >> fw >> fh;
    if (fw != FACE_W || fh != FACE_H) {
        qWarning() << "FaceRecognizer::loadModel: dimension mismatch in saved model.";
        return false;
    }
    m_numComponents = numComp;

    // Mean face
    qint32 sz;
    ds >> sz; m_mean.resize(sz);
    for (float &v : m_mean) ds >> v;

    // Eigenfaces
    ds >> sz; m_eigenfaces.resize(sz);
    for (auto &ef : m_eigenfaces) {
        qint32 esz; ds >> esz; ef.resize(esz);
        for (float &v : ef) ds >> v;
    }

    // Eigenvalues (may be absent in files saved by older versions — handle gracefully)
    if (!ds.atEnd()) {
        ds >> sz; m_eigenvalues.resize(sz);
        for (float &v : m_eigenvalues) ds >> v;
    }

    // Training projections + labels
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