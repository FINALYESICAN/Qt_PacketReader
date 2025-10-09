#ifndef SESSIONFORM_H
#define SESSIONFORM_H

#include <QWidget>
#include <QHash>
#include <QVector>
#include <QJsonObject>
#include "sessionmodel.h"

namespace Ui {
class SessionForm;
}

class SessionForm : public QWidget
{
    Q_OBJECT

public:
    explicit SessionForm(QWidget *parent = nullptr);
    ~SessionForm();

public slots:
    void onFlow(const QJsonObject& evt);
    void onSummary(const QJsonObject& evt);

private:
    Ui::SessionForm *ui;
    SessionModel* m_model = nullptr;
};

#endif // SESSIONFORM_H
