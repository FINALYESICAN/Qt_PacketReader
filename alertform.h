#ifndef ALERTFORM_H
#define ALERTFORM_H

#include <QWidget>

namespace Ui {
class alertForm;
}

class alertForm : public QWidget
{
    Q_OBJECT

public:
    explicit alertForm(QWidget *parent = nullptr);
    ~alertForm();

private:
    Ui::alertForm *ui;
};

#endif // ALERTFORM_H
