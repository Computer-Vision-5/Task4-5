#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <QImage>
#include <QString>
#include <QStringList>
#include <vector>
#include <string>

class FaceRecognizer
{
public:
    FaceRecognizer();

    // ── A. Preprocessing — PCA-ready face extraction ────────────────────────
    //
    //  detectAndCrop(img)   → crop the largest detected face from the image
    //  preprocessFace(crop) → grayscale, resize to FACE_H×FACE_W, flatten
    //                         to a 1-D float vector of length FACE_DIM.

    QImage             detectAndCrop(const QImage &img) const;
    std::vector<float> preprocessFace(const QImage &crop) const;

    // ── B.1  Training (one-time, at startup) ────────────────────────────────

    /**
     * Train the PCA/Eigenfaces model from a folder of images.
     *
     * Image naming: <PersonID>_<anything>.<ext>
     * PersonID = everything before the first '_'.
     *
     * @param trainPath          Folder with training images (~80% of dataset).
     * @param varianceThreshold  Cumulative-variance fraction to retain
     *                           (e.g. 0.95 keeps enough components for 95%).
     *                           Pass 0.0 to keep all N-1 components.
     * @return                   Number of faces successfully loaded.
     */
    int trainFromFolder(const QString &trainPath,
                        double varianceThreshold = 0.95);

    /**
     * Convenience: train AND save the model in one call.
     *
     * @param trainDataDir       Folder with training images.
     * @param modelPath          Destination path for the binary model file.
     * @param varianceThreshold  Forwarded to trainFromFolder().
     * @return                   true if training succeeded and model was saved.
     */
    bool trainAndSaveModel(const QString &trainDataDir,
                           const QString &modelPath,
                           double varianceThreshold = 0.95);

    // ── B.2  Recognition (load model once, never retrain) ───────────────────

    QString recognize(QImage &image, double &confidence);
    QString recognize(QImage &image);

    // ── Persistence ─────────────────────────────────────────────────────────

    /** Save mean_face, eigenvectors (K×D), eigenvalues (K),
     *  X_train_pca (N×K), y_train (N), and config (W, H, K). */
    bool saveModel(const QString &filePath) const;

    /** Load a saved model.  After this, recognize() works immediately. */
    bool loadModel(const QString &filePath);

    // ── Accessors ───────────────────────────────────────────────────────────

    bool   isTrained()        const { return m_trained; }
    int    numSubjects()      const;
    int    numComponents()    const { return m_numComponents; }
    int    getTrainingCount() const { return static_cast<int>(m_projections.size()); }

    // ── Constants ───────────────────────────────────────────────────────────

    static constexpr int FACE_W   = 100;
    static constexpr int FACE_H   = 100;
    static constexpr int FACE_DIM = FACE_W * FACE_H;

private:
    // Computes PCA using the Turk-Pentland small-matrix trick.
    // K is chosen automatically so retained eigenvalues cover
    // at least m_varianceThreshold of total variance.
    void computePCA();

    std::vector<float> project(const std::vector<float> &face) const;
    static float euclidean(const std::vector<float> &a, const std::vector<float> &b);
    QString extractPersonId(const QString &filename) const;

    // ── Model state ─────────────────────────────────────────────────────────

    bool                             m_trained           = false;
    int                              m_numComponents     = 0;
    double                           m_varianceThreshold = 0.95;

    std::vector<float>               m_mean;          // mean face     (D)
    std::vector<std::vector<float>>  m_eigenfaces;    // eigenvectors  (K × D)
    std::vector<float>               m_eigenvalues;   // eigenvalues   (K)
    std::vector<std::vector<float>>  m_projections;   // X_train_pca   (N × K)
    std::vector<std::string>         m_labels;        // y_train       (N)

    // Temporary — only alive during training, freed immediately after
    // computePCA() returns to avoid holding O(N × D) bytes permanently.
    std::vector<std::vector<float>>  m_trainingFaces;
};

#endif // FACERECOGNIZER_H