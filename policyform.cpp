#include "policyform.h"
#include "ui_policyform.h"

policyForm::policyForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::policyForm)
{
    ui->setupUi(this);
}

policyForm::~policyForm()
{
    delete ui;
}
