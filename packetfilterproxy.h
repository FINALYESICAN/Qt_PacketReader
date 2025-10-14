#ifndef PACKETFILTERPROXY_H
#define PACKETFILTERPROXY_H

#include <QObject>
#include <QSortFilterProxyModel>

class packetfilterproxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit packetfilterproxy(QObject* parent=nullptr) : QSortFilterProxyModel(parent) {
        setDynamicSortFilter(true);
    }

    // 빈 문자열이면 그 조건은 무시 (부분 문자열 매칭)
    void setSrcIp  (const QString& s){ m_srcIp   = s.trimmed(); invalidateFilter(); }
    void setSrcPort(const QString& s){ m_srcPort = s.trimmed(); invalidateFilter(); }
    void setDstIp  (const QString& s){ m_dstIp   = s.trimmed(); invalidateFilter(); }
    void setDstPort(const QString& s){ m_dstPort = s.trimmed(); invalidateFilter(); }
    void setOrLogic(bool on){ m_orLogic = on; invalidateFilter(); }

protected:
    bool filterAcceptsRow(int r, const QModelIndex& p) const override {
        const auto* M = sourceModel();

        // PacketModel의 컬럼 정의: Time,Dir,Proto,Src,Sport,Dst,Dport,... (Src=3, Sport=4, Dst=5, Dport=6)
        const QString src = M->data(M->index(r, 2, p), Qt::DisplayRole).toString();
        const QString dst = M->data(M->index(r, 4, p), Qt::DisplayRole).toString();
        const int     sp  = M->data(M->index(r, 3, p), Qt::DisplayRole).toInt();
        const int     dp  = M->data(M->index(r, 5, p), Qt::DisplayRole).toInt();

        const bool srcIpOk   = m_srcIp.isEmpty()   ? true : src.contains(m_srcIp, Qt::CaseInsensitive);
        const bool srcPortOk = m_srcPort.isEmpty() ? true : QString::number(sp).contains(m_srcPort);
        const bool dstIpOk   = m_dstIp.isEmpty()   ? true : dst.contains(m_dstIp, Qt::CaseInsensitive);
        const bool dstPortOk = m_dstPort.isEmpty() ? true : QString::number(dp).contains(m_dstPort);

        const bool hasSrc = !m_srcIp.isEmpty() || !m_srcPort.isEmpty();
        const bool hasDst = !m_dstIp.isEmpty() || !m_dstPort.isEmpty();

        const bool srcGroup = hasSrc ? (srcIpOk && srcPortOk) : true;  // 그룹 내 AND
        const bool dstGroup = hasDst ? (dstIpOk && dstPortOk) : true;

        if (!hasSrc && !hasDst) return true;          // 아무 조건도 없으면 전체 표시
        if ( hasSrc &&  hasDst)                        // 둘 다 지정된 경우: OR/AND 토글
            return m_orLogic ? (srcGroup || dstGroup)  // OR 모드
                             : (srcGroup && dstGroup); // AND 모드
        // 하나만 지정된 경우: 그 그룹 결과만 따르면 됨
        return hasSrc ? srcGroup : dstGroup;
    }

private:
    QString m_srcIp, m_srcPort, m_dstIp, m_dstPort;
    bool m_orLogic = false;
};

#endif // PACKETFILTERPROXY_H
