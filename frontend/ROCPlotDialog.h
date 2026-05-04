#ifndef ROCPLOTDIALOG_H
#define ROCPLOTDIALOG_H

#include <QDialog>
#include <vector>
#include <utility>

class ROCPlotDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ROCPlotDialog(const std::vector<std::pair<double, double>>& rocPoints, double auc, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<std::pair<double, double>> m_rocPoints;
    double m_auc;
};

#endif // ROCPLOTDIALOG_H
