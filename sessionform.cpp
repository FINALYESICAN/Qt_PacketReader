#include "sessionform.h"
#include "ui_sessionform.h"
#include <QHeaderView>

SessionForm::SessionForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SessionForm)
{
    ui->setupUi(this);
    m_model = new SessionModel(this);
    ui->tableSessions->setModel(m_model);
    ui->tableSessions->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableSessions->setSortingEnabled(true);

}

SessionForm::~SessionForm()
{
    delete ui;
}

void SessionForm::onFlow(const QJsonObject& evt) {
    // SessionModel의 업데이트 로직 그대로 활용
    m_model->upsertFromJson(evt);  // FLOW_UPDATE/END 처리
}

void SessionForm::onSummary(const QJsonObject& evt) {
    m_model->replaceFromSummary(evt); // SUMMARY 처리
}
