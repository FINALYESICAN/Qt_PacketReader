#include "alertform.h"
#include "ui_alertform.h"

alertForm::alertForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::alertForm)
{
    ui->setupUi(this);
}

alertForm::~alertForm()
{
    delete ui;
}
