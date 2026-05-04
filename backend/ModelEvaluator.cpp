#include "ModelEvaluator.h"
#include "FaceRecognizer.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QDebug>
#include <algorithm>

EvaluationResult ModelEvaluator::evaluate(FaceRecognizer& recognizer, const QString& testPath)
{
    EvaluationResult result;
    result.accuracy = 0.0;
    result.totalImages = 0;
    result.correctImages = 0;
    result.auc = 0.0;

    if (!recognizer.isTrained()) {
        qWarning() << "ModelEvaluator: Recognizer is not trained.";
        return result;
    }

    QDir dir(testPath);
    if (!dir.exists()) {
        qWarning() << "ModelEvaluator: Test folder not found:" << testPath;
        return result;
    }

    QStringList filters = {"*.jpg","*.jpeg","*.png","*.bmp","*.tif","*.tiff"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    // List of predictions: (is_correct, confidence)
    std::vector<std::pair<bool, double>> predictions;

    for (const QFileInfo &fi : files) {
        QString filename = fi.fileName();
        QString trueLabel = filename.section('_', 0, 0);

        QImage img(fi.absoluteFilePath());
        if (img.isNull()) continue;

        double confidence = 0.0;
        QString predictedLabel = recognizer.recognize(img, confidence);
        
        bool isCorrect = (!predictedLabel.isEmpty() && predictedLabel == trueLabel);
        predictions.push_back({isCorrect, confidence});
        
        if (isCorrect) {
            result.correctImages++;
        }
        result.totalImages++;
    }

    if (result.totalImages > 0) {
        result.accuracy = static_cast<double>(result.correctImages) / result.totalImages;
    }

    // Compute ROC Curve points
    std::sort(predictions.begin(), predictions.end(), 
        [](const std::pair<bool, double>& a, const std::pair<bool, double>& b) {
            return a.second > b.second;
        }
    );

    int totalPositives = result.totalImages; 
    int totalNegatives = result.totalImages;

    int tp = 0;
    int fp = 0;
    
    result.rocPoints.push_back({0.0, 0.0});
    
    double auc = 0.0;
    double prevFpr = 0.0;
    double prevTpr = 0.0;

    for (const auto& p : predictions) {
        if (p.first) {
            tp++;
        } else {
            fp++;
        }
        double tpr = static_cast<double>(tp) / totalPositives;
        double fpr = static_cast<double>(fp) / totalNegatives;
        
        result.rocPoints.push_back({fpr, tpr});
        
        // Trapezoidal rule for AUC
        auc += (fpr - prevFpr) * (tpr + prevTpr) / 2.0;
        
        prevFpr = fpr;
        prevTpr = tpr;
    }

    if (prevFpr < 1.0) {
        auc += (1.0 - prevFpr) * (1.0 + prevTpr) / 2.0;
        result.rocPoints.push_back({1.0, 1.0});
    }

    result.auc = auc;
    return result;
}
