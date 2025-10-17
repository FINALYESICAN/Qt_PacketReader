#ifndef REPORTFORM_H
#define REPORTFORM_H

#include <QWidget>
#include <QJsonObject>

namespace Ui {
class ReportForm;
}

class ReportForm : public QWidget
{
    Q_OBJECT

public:
    explicit ReportForm(QWidget *parent = nullptr);
    ~ReportForm();

public slots:
    void onReport(const QJsonObject& rep);

private:
    Ui::ReportForm *ui;
    static QString humanBytes(qulonglong bytes);
};

#endif // REPORTFORM_H
