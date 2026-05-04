#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPainter>
#include <QDir>
#include <QMessageBox>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include <QPushButton>
#include "../backend/FaceDetector.h"
#include "../backend/ModelEvaluator.h"
#include "ROCPlotDialog.h"

static QString makeButtonStyle(const QString &bg, const QString &hover, const QString &press)
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "  padding: 10px 20px;"
        "}"
        "QPushButton:hover  { background-color: %2; }"
        "QPushButton:pressed{ background-color: %3; }"
        "QPushButton:disabled{ background-color: #9e9e9e; }")
        .arg(bg, hover, press);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Face Detection & Recognition – Eigenfaces");
    setMinimumSize(960, 740);

    QWidget *central = new QWidget(this);
    central->setStyleSheet("QWidget { background-color: #f0f2f5; }");
    setCentralWidget(central);

    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(32, 32, 32, 24);
    root->setSpacing(18);

    // ── Button row ───────────────────────────────────────────────────────────
    trainButton  = new QPushButton("Train on Dataset",        this);
    uploadButton = new QPushButton("Recognize Single Image",  this);
    evalButton   = new QPushButton("Evaluate Test Set",       this);

    trainButton ->setStyleSheet(makeButtonStyle("#198754", "#157347", "#146c43"));
    uploadButton->setStyleSheet(makeButtonStyle("#0d6efd", "#0b5ed7", "#0a58ca"));
    evalButton  ->setStyleSheet(makeButtonStyle("#ffc107", "#ffb300", "#ff8f00"));
    evalButton  ->setStyleSheet(evalButton->styleSheet() + "QPushButton { color: #212529; }"); // Dark text for yellow button

    uploadButton->setEnabled(false);
    evalButton->setEnabled(false);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(trainButton);
    btnRow->addSpacing(12);
    btnRow->addWidget(uploadButton);
    btnRow->addSpacing(12);
    btnRow->addWidget(evalButton);
    btnRow->addStretch();
    root->addLayout(btnRow);

    // ── Progress bar ─────────────────────────────────────────────────────────
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0);
    progressBar->setVisible(false);
    progressBar->setFixedHeight(8);
    progressBar->setStyleSheet(
        "QProgressBar { border:none; background:#dee2e6; border-radius:4px; }"
        "QProgressBar::chunk { background:#0d6efd; border-radius:4px; }");
    root->addWidget(progressBar);

    // ── Result label ─────────────────────────────────────────────────────────
    resultLabel = new QLabel("Welcome! Select a training folder or load an existing model.", this);
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setWordWrap(true);
    resultLabel->setStyleSheet(
        "QLabel { background:#ffffff; color:#495057; font-size:14px;"
        "         border:1px solid #dee2e6; border-radius:8px; padding:8px; }");
    root->addWidget(resultLabel);

    // ── Image display ────────────────────────────────────────────────────────
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(860, 520);
    imageLabel->setText("No image loaded");
    imageLabel->setStyleSheet(
        "QLabel { background:#ffffff; color:#adb5bd; font-size:18px;"
        "         border:2px dashed #ced4da; border-radius:12px; }");
    root->addWidget(imageLabel);
    root->setStretchFactor(imageLabel, 1);

    statusBar()->showMessage("Ready");

    // ── Connections ──────────────────────────────────────────────────────────
    connect(trainButton,  &QPushButton::clicked, this, &MainWindow::trainModel);
    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::uploadImage);
    connect(evalButton,   &QPushButton::clicked, this, &MainWindow::evaluateModel);

    // Try to load a previously saved model as soon as the event loop starts
    QTimer::singleShot(0, this, &MainWindow::loadExistingModel);
}

MainWindow::~MainWindow() = default;

// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::loadExistingModel()
{
    QString modelPath = QApplication::applicationDirPath() + "/face_model.dat";

    if (m_recognizer.loadModel(modelPath)) {
        resultLabel->setText(
            QString("Model loaded!\n"
                    "Training images: %1  |  Subjects: %2\n"
                    "Ready to test or recognize faces.")
            .arg(m_recognizer.getTrainingCount())
            .arg(m_recognizer.numSubjects()));
        statusBar()->showMessage("Model loaded from " + modelPath);
        uploadButton->setEnabled(true);
        evalButton->setEnabled(true);
    } else {
        resultLabel->setText("No saved model found. Please train on a dataset folder.");
        statusBar()->showMessage("No model found. Please train first.");
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::trainModel()
{
    QString trainDir = QFileDialog::getExistingDirectory(
        this, "Select Training Folder (contains personID_filename.jpg)",
        QDir::currentPath());

    if (trainDir.isEmpty()) return;
    m_trainPath = trainDir;

    trainButton ->setEnabled(false);
    uploadButton->setEnabled(false);
    evalButton  ->setEnabled(false);
    progressBar ->setVisible(true);
    statusBar()->showMessage("Training… please wait");
    resultLabel->setText("Training PCA model – this may take a minute…");

    FaceRecognizer *rec       = &m_recognizer;
    QString         trainPath = m_trainPath;

    auto *watcher = new QFutureWatcher<int>(this);
    connect(watcher, &QFutureWatcher<int>::finished, this, [=]() {
        int loaded = watcher->result();
        progressBar->setVisible(false);
        trainButton->setEnabled(true);

        if (loaded < 2) {
            resultLabel->setText("Training failed – no faces found in the selected folder.");
            statusBar()->showMessage("Training failed.");
        } else {
            int subjects = rec->numSubjects();

            QString modelPath = QApplication::applicationDirPath() + "/face_model.dat";
            if (rec->saveModel(modelPath)) {
                resultLabel->setText(
                    QString("Model trained on %1 face images from %2 subjects!\n"
                            "PCA components retained: %3\n"
                            "Model saved to: %4\n"
                            "You can now recognize single images.")
                    .arg(loaded)
                    .arg(subjects)
                    .arg(rec->numComponents())
                    .arg(modelPath));
                statusBar()->showMessage(
                    QString("Trained: %1 images, %2 subjects, %3 components, model saved")
                    .arg(loaded).arg(subjects).arg(rec->numComponents()));
                uploadButton->setEnabled(true);
                evalButton->setEnabled(true);
            } else {
                resultLabel->setText("Model trained but failed to save to disk.");
                uploadButton->setEnabled(true);
                evalButton->setEnabled(true);
            }
        }
        watcher->deleteLater();
    });

    // Pass varianceThreshold = 0.95 (keep components that explain 95% of variance).
    // This matches the header default and replaces the old hard-coded "50 components".
    watcher->setFuture(
        QtConcurrent::run([rec, trainPath]() -> int {
            return rec->trainFromFolder(trainPath, 0.95);
        })
    );
}

void MainWindow::uploadImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open Query Image",
        QDir::currentPath(),
        "Image Files (*.png *.jpg *.jpeg *.bmp)");

    if (fileName.isEmpty()) return;

    QImage image(fileName);
    if (image.isNull()) {
        resultLabel->setText("Failed to load image.");
        return;
    }

    // ── Face detection (green boxes) ─────────────────────────────────────────
    FaceDetector::processImage(image);

    // ── Face recognition (blue box + label), if a model is loaded ────────────
    QString recognitionText = "Detection complete (no model loaded).";
    if (m_recognizer.isTrained()) {
        // Load a fresh copy so recognition can draw its own overlay independently
        QImage imgForRecog(fileName);
        double  confidence  = 0.0;
        QString personId    = m_recognizer.recognize(imgForRecog, confidence);

        if (!personId.isEmpty()) {
            // Composite the recognition overlay onto the detection image
            QPainter p(&image);
            p.drawImage(0, 0, imgForRecog);

            recognitionText = QString("Recognized: <b>%1</b>  —  confidence: %2%")
                .arg(personId)
                .arg(static_cast<int>(confidence * 100));
        } else {
            recognitionText = "No face detected in query image for recognition.";
        }
    }

    resultLabel->setText(recognitionText);

    QPixmap pix = QPixmap::fromImage(image);
    imageLabel->setPixmap(
        pix.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    statusBar()->showMessage("Done – " + QFileInfo(fileName).fileName());
}

void MainWindow::evaluateModel()
{
    QString testDir = QFileDialog::getExistingDirectory(
        this, "Select Test Folder (contains personID_filename.jpg)",
        QDir::currentPath());

    if (testDir.isEmpty()) return;

    trainButton ->setEnabled(false);
    uploadButton->setEnabled(false);
    evalButton  ->setEnabled(false);
    progressBar ->setVisible(true);
    statusBar()->showMessage("Evaluating test set… please wait");
    resultLabel->setText("Evaluating model on test set. This may take a minute…");

    FaceRecognizer *rec = &m_recognizer;

    auto *watcher = new QFutureWatcher<EvaluationResult>(this);
    connect(watcher, &QFutureWatcher<EvaluationResult>::finished, this, [=]() {
        EvaluationResult result = watcher->result();
        progressBar->setVisible(false);
        trainButton->setEnabled(true);
        uploadButton->setEnabled(true);
        evalButton->setEnabled(true);

        if (result.totalImages == 0) {
            resultLabel->setText("Evaluation failed – no faces found in the selected folder.");
            statusBar()->showMessage("Evaluation failed.");
        } else {
            resultLabel->setText(
                QString("Evaluation Complete!\n"
                        "Total Images: %1  |  Correct: %2\n"
                        "Accuracy: %3%  |  AUC: %4")
                .arg(result.totalImages)
                .arg(result.correctImages)
                .arg(static_cast<int>(result.accuracy * 100))
                .arg(result.auc, 0, 'f', 3));
            statusBar()->showMessage("Evaluation complete.");

            // Show ROC curve dialog
            ROCPlotDialog dlg(result.rocPoints, result.auc, this);
            dlg.exec();
        }
        watcher->deleteLater();
    });

    watcher->setFuture(
        QtConcurrent::run([rec, testDir]() -> EvaluationResult {
            return ModelEvaluator::evaluate(*rec, testDir);
        })
    );
}