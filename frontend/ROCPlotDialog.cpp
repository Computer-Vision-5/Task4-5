#include "ROCPlotDialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QFont>
#include <QPaintEvent>

ROCPlotDialog::ROCPlotDialog(const std::vector<std::pair<double, double>>& rocPoints, double auc, QWidget *parent)
    : QDialog(parent), m_rocPoints(rocPoints), m_auc(auc)
{
    setWindowTitle("ROC Curve");
    setMinimumSize(600, 600);
    setStyleSheet("QDialog { background-color: #ffffff; }");
}

void ROCPlotDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int margin = 60;
    int w = width() - 2 * margin;
    int h = height() - 2 * margin;

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(margin, margin + h, margin + w, margin + h); // X axis
    painter.drawLine(margin, margin, margin, margin + h);         // Y axis

    // Draw labels
    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(margin + w / 2 - 80, margin + h + 45, "False Positive Rate (FPR)");
    
    painter.save();
    painter.translate(margin - 40, margin + h / 2 + 80);
    painter.rotate(-90);
    painter.drawText(0, 0, "True Positive Rate (TPR)");
    painter.restore();

    // Draw grid & ticks
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    painter.setFont(QFont("Arial", 9));
    for (int i = 0; i <= 10; ++i) {
        int x = margin + (w * i) / 10;
        int y = margin + h - (h * i) / 10;
        
        // Vertical grid
        painter.drawLine(x, margin, x, margin + h);
        // Horizontal grid
        painter.drawLine(margin, y, margin + w, y);
        
        // Ticks text
        painter.setPen(QPen(Qt::black, 1));
        painter.drawText(x - 10, margin + h + 18, QString::number(i / 10.0, 'f', 1));
        painter.drawText(margin - 30, y + 5, QString::number(i / 10.0, 'f', 1));
        painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    }

    // Draw random guess line (dashed)
    painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
    painter.drawLine(margin, margin + h, margin + w, margin);

    // Draw ROC curve
    if (m_rocPoints.size() > 1) {
        QPainterPath path;
        path.moveTo(margin + m_rocPoints[0].first * w, margin + h - m_rocPoints[0].second * h);
        for (size_t i = 1; i < m_rocPoints.size(); ++i) {
            path.lineTo(margin + m_rocPoints[i].first * w, margin + h - m_rocPoints[i].second * h);
        }
        painter.setPen(QPen(Qt::blue, 3));
        painter.drawPath(path);
    }

    // Draw AUC text
    painter.setPen(QPen(Qt::black, 2));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(margin + w - 160, margin + h - 30, QString("AUC: %1").arg(m_auc, 0, 'f', 3));
}
