#ifndef ALERTFORM_H
#define ALERTFORM_H

#include <QWidget>
#include <QJsonObject>
#include "alertmodel.h"

namespace Ui {
class alertForm;
}

class alertForm : public QWidget
{
    Q_OBJECT

public:
    explicit alertForm(QWidget *parent = nullptr);
    ~alertForm();

public slots:
    void onAlert(const QJsonObject& evt);

private:
    Ui::alertForm *ui;
    AlertModel* m_model = nullptr;
    void appendWithScrollGuard(const QJsonObject& evt);
};

#endif // ALERTFORM_H
