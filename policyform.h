#ifndef POLICYFORM_H
#define POLICYFORM_H

#include <QWidget>

namespace Ui {
class policyForm;
}

class policyForm : public QWidget
{
    Q_OBJECT

public:
    explicit policyForm(QWidget *parent = nullptr);
    ~policyForm();

private:
    Ui::policyForm *ui;
};

#endif // POLICYFORM_H
