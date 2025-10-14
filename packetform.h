#ifndef PACKETFORM_H
#define PACKETFORM_H

#include <QWidget>
#include <QJsonObject>
#include "packetmodel.h"
#include "packetfilterproxy.h"

namespace Ui {
class PacketForm;
}

class PacketForm : public QWidget
{
    Q_OBJECT

public:
    explicit PacketForm(QWidget *parent = nullptr);
    ~PacketForm();

signals:
    void packetReq(QJsonObject req);

public slots:
    void onPacket(const QJsonObject& evt);
    void onPacketData(const QJsonObject& data);

private slots:
    void on_tablePacket_doubleClicked(const QModelIndex &index);

    void on_FilterButton_clicked();

    void on_ResetButton_clicked();

private:
    Ui::PacketForm *ui;
    PacketModel* m_model = nullptr;
    packetfilterproxy* m_proxy = nullptr;
    void appendWithScrollGuard(const QJsonObject& evt);
};

#endif // PACKETFORM_H
