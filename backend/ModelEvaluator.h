#ifndef MODELEVALUATOR_H
#define MODELEVALUATOR_H

#include <QString>
#include <vector>
#include <utility>

class FaceRecognizer; // Forward declaration

struct EvaluationResult {
    double accuracy;
    int totalImages;
    int correctImages;
    double auc; // Area Under Curve
    std::vector<std::pair<double, double>> rocPoints; // (FPR, TPR)
};

class ModelEvaluator
{
public:
    static EvaluationResult evaluate(FaceRecognizer& recognizer, const QString& testPath);
};

#endif // MODELEVALUATOR_H
