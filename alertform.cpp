#include "alertform.h"
#include "ui_alertform.h"
#include <QHeaderView>

alertForm::alertForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::alertForm)
{
    ui->setupUi(this);

    m_model = new AlertModel(this);
    ui->tableAlerts->setModel(m_model);
    ui->tableAlerts->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableAlerts->setSortingEnabled(true);
}

alertForm::~alertForm()
{
    delete ui;
}

void alertForm::onAlert(const QJsonObject& evt) {
    // type 필터링 (혹시 다른 타입이 들어올 수도 있으니까)
    if (evt["type"].toString() != "ALERT") return;
    m_model->appendFromJson(evt);
}
