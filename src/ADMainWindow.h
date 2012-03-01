#ifndef ADMAINWINDOW_H
#define ADMAINWINDOW_H

#include "ui_ADMainWindow.h"
#include <QMainWindow>

#include <ADConnection.h>
#include <ADSubscription.h>
#include <ADOrder.h>

class QStandardItemModel;

class ADMainWindow : public QMainWindow, public Ui::ADMainWindow
{
    Q_OBJECT
public:
    ADMainWindow ();

private:
    friend class ADTableView;
    void setupModel ();
    void setupViews ();

    void closeEvent ( QCloseEvent* );
    void keyPressEvent ( QKeyEvent* );
    void mousePressEventADTableView ( ADTableView*, QMouseEvent* );
    void keyPressEventADTableView (  ADTableView* view, QKeyEvent* ke );

private slots:
    void onConnectClick ();
    void onFindClick ();
    void onConnectionStateChanged ( ADConnection::State );
    void onSpinValueChanged ( int );
    void onMarketsChange ( int );
    void onQuoteReceived ( int paperNo, ADConnection::Subscription::Type );
    void onHistoricalQuotesReceived ( ADConnection::Request, QVector<ADConnection::HistoricalQuote> );
    void onPositionChanged ( QString accCode, int paperNo );
    void onOrderStateChanged ( ADConnection::Order,
                               ADConnection::Order::State,
                               ADConnection::Order::State );
    void onOrderOperationResult ( ADConnection::Order,
                                  ADConnection::Order::Operation,
                                  ADConnection::Order::OperationResult );
    void onTrade ( ADConnection::Order, quint32 qty );
    void onEverySecond ();

private:
    QStandardItemModel* m_sellersTableModel;
    QStandardItemModel* m_buyersTableModel;
    ADConnection m_adConnect;
    ADConnection::Subscription m_sub;
    ADConnection::Position m_market;
    int m_papNo;
    QTimer m_everySecondTimer;
    QList<ADConnection::Order> m_sellOrders;
    QList<ADConnection::Order> m_buyOrders;
    qint32 m_pos;
};



#endif // ADMAINWINDOW_H
