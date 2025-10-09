#ifndef PACKETFORM_H
#define PACKETFORM_H

#include <QWidget>
#include <QJsonObject>
#include "packetmodel.h"

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

private:
    Ui::PacketForm *ui;
    PacketModel* m_model = nullptr;
};

#endif // PACKETFORM_H
