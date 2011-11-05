#include <QtGui>

#include "ADMainWindow.h"

#define PLAIN_QUEUE
#ifndef PLAIN_QUEUE
 #define TBL_HEADERS 4
 #define TBL_ORDER_IND 3
 #define TBL_PRICE_IND 1
 #define TBL_SELL_IND 2
 #define TBL_BUY_IND 0
#else
 #define TBL_HEADERS 3
 #define TBL_ORDER_IND 2
 #define TBL_PRICE_IND 1
 #define TBL_SELL_IND 0
 #define TBL_BUY_IND 0
#endif

/***************************************************************************************/

ADMainWindow::ADMainWindow ( const QString& login,
                             const QString& passwd,
                             const QString& accCode,
                             const QString& papCode ) :
    m_login(login),
    m_passwd(passwd),
    m_accCode(accCode),
    m_papCode(papCode),
    m_papNo(0),
    m_pos(0)
{
    setupUi(this);
    setupModel();
    setupViews();

    statusBar();

    QObject::connect(Ui_ADMainWindow::connectButton,
                     SIGNAL(clicked()),
                     SLOT(onConnectClick()));

    QObject::connect(&m_adConnect,
                     SIGNAL(onStateChanged(ADConnection::State)),
                     SLOT(onConnectionStateChanged(ADConnection::State)));

    QObject::connect(&m_adConnect,
                     SIGNAL(onQuoteReceived(int,ADConnection::Subscription::Type)),
                     SLOT(onQuoteReceived(int,ADConnection::Subscription::Type)));

    QObject::connect(&m_adConnect,
                     SIGNAL(onHistoricalQuotesReceived(ADConnection::Request,
                                                       QVector<ADConnection::HistoricalQuote>)),
                     SLOT(onHistoricalQuotesReceived(ADConnection::Request,
                                                     QVector<ADConnection::HistoricalQuote>)));


    QObject::connect(&m_adConnect,
                     SIGNAL(onPositionChanged(QString,QString,int)),
                     SLOT(onPositionChanged(QString,QString,int)));

    QObject::connect(&m_adConnect,
                     SIGNAL(onOrderStateChanged(ADConnection::Order,
                                                ADConnection::Order::State,
                                                ADConnection::Order::State)),
                     SLOT(onOrderStateChanged(ADConnection::Order,
                                              ADConnection::Order::State,
                                              ADConnection::Order::State)));

    QObject::connect(&m_adConnect,
                     SIGNAL(onOrderOperationResult(ADConnection::Order,
                                                   ADConnection::Order::Operation,
                                                   ADConnection::Order::OperationResult)),
                     SLOT(onOrderOperationResult(ADConnection::Order,
                                                 ADConnection::Order::Operation,
                                                 ADConnection::Order::OperationResult)));

    QObject::connect(&m_adConnect,
                     SIGNAL(onTrade(ADConnection::Order, quint32)),
                     SLOT(onTrade(ADConnection::Order, quint32)));

    QObject::connect( &m_everySecondTimer,
                      SIGNAL(timeout()),
                      SLOT(onEverySecond()) );

    m_everySecondTimer.start(1000);

    setWindowTitle(tr("AlfaDirect DOM Scalp"));
    resize(230, 600);
    setFixedWidth(230);
}

void ADMainWindow::setupModel()
{
    m_sellersTableModel = new QStandardItemModel(0, TBL_HEADERS, this);
    m_buyersTableModel = new QStandardItemModel(0, TBL_HEADERS, this);
}

void ADMainWindow::setupViews()
{
    Ui_ADMainWindow::sellersTableView->setADMainWindow(this);
    Ui_ADMainWindow::sellersTableView->setModel(m_sellersTableModel);
    Ui_ADMainWindow::buyersTableView->setADMainWindow(this);
    Ui_ADMainWindow::buyersTableView->setModel(m_buyersTableModel);

    Ui_ADMainWindow::sellersTableView->setColumnWidth(0, 40);
    Ui_ADMainWindow::sellersTableView->setColumnWidth(1, 50);
    Ui_ADMainWindow::sellersTableView->setColumnWidth(2, 40);
    Ui_ADMainWindow::buyersTableView->setColumnWidth(0, 40);
    Ui_ADMainWindow::buyersTableView->setColumnWidth(1, 50);
    Ui_ADMainWindow::buyersTableView->setColumnWidth(2, 40);
}

void ADMainWindow::onConnectClick ()
{
    if ( m_adConnect.isConnected() ) {
        Ui_ADMainWindow::connectButton->setText("Disconnecting ...");
        Ui_ADMainWindow::connectButton->setDisabled(true);
        m_adConnect.disconnect();
    }
    else {
        Ui_ADMainWindow::connectButton->setText("Connecting ...");
        Ui_ADMainWindow::connectButton->setDisabled(true);
        QFile auth( QCoreApplication::applicationDirPath() + "/ad_auth" );
        bool res = auth.open( QFile::ReadOnly );
        if ( ! res ) {
            Ui_ADMainWindow::connectButton->setText("Connect");
            Ui_ADMainWindow::connectButton->setDisabled(false);
            qWarning("can't open auth file!");
            return;
        }

        QString login = QString(auth.readLine()).remove( QRegExp("(\\r|\\n)+$") );
        QString passwd = QString(auth.readLine()).remove( QRegExp("(\\r|\\n)+$") );
        auth.close();

        m_adConnect.connect( login, passwd );
    }
}

void ADMainWindow::onConnectionStateChanged ( ADConnection::State st )
{
    if ( st == ADConnection::ConnectedState ) {
        if ( m_papNo == 0 ) {
            bool res = m_adConnect.findPaperNo( m_papCode, false, m_papNo );
            if ( ! res ) {
                qWarning("Can't find paper '%s'", qPrintable(m_papCode));
                m_adConnect.disconnect();
                return;
            }
        }

        Ui_ADMainWindow::connectButton->setText("Disconnect");
        Ui_ADMainWindow::connectButton->setDisabled(false);

        ADConnection::Subscription::Options subscr(
            QSet<int>() << m_papNo,
            // To be waken up
            ADConnection::Subscription::QuoteSubscription |
            ADConnection::Subscription::QueueSubscription,
            // To receive from server
            ADConnection::Subscription::QuoteSubscription |
            ADConnection::Subscription::QueueSubscription);

        QList<ADConnection::Subscription::Options> subscrOpts =
            QList<ADConnection::Subscription::Options>() << subscr;
        // Full subscribe
        m_sub = m_adConnect.subscribeToQuotes( subscrOpts );
        if ( ! m_sub ) {
            qWarning("Can't subscribe to queues!");
            return;
        }
    }
    else {
        Ui_ADMainWindow::connectButton->setText("Connect");
        Ui_ADMainWindow::connectButton->setDisabled(false);
        m_sub = ADConnection::Subscription();
    }
}

void ADMainWindow::onQuoteReceived ( int paperNo,
                                     ADConnection::Subscription::Type subType )
{
    ADConnection::Quote q;
    m_adConnect.getQuote( paperNo, q );

    if ( subType == ADConnection::Subscription::QueueSubscription ) {
        if ( paperNo == m_papNo ) {
            int rowHeight = Ui_ADMainWindow::buyersTableView->verticalHeader()->defaultSectionSize();
            QSize viewSize = Ui_ADMainWindow::buyersTableView->size();
            int maxRows = (viewSize.height() / rowHeight) + 5;
            maxRows = (maxRows > 0 ? maxRows : 0);

            QHash<quint64, QString> sellOrders;
            foreach ( ADConnection::Order order, m_sellOrders ) {
                quint64 price = static_cast<quint64>(order.getOrderPrice());
                sellOrders[price] += "*";
            }
            QHash<quint64, QString> buyOrders;
            foreach ( ADConnection::Order order, m_buyOrders ) {
                quint64 price = static_cast<quint64>(order.getOrderPrice());
                buyOrders[price] += "*";
            }

            // Buyers
            {
                int firstQty = 0;
                float firstPrice = 0.0;
                QMap<float, int>::Iterator it = q.buyers.end() - 1;
                for ( int i = 0; it != q.buyers.begin() - 1 && i < maxRows && q.buyers.size() > 0;
                      --it, ++i ) {
                    if ( m_buyersTableModel->rowCount() <= i )
                        m_buyersTableModel->insertRow(i, QModelIndex());

                    float price = it.key();
                    QString priceStr = QString("%1").arg(price);
                    int priceInt = static_cast<int>(price);
                    QString priceIntStr = QString("%1").arg(priceInt);
                    if ( priceInt < price )
                        priceIntStr = priceStr;
                    else {
                        QString str;
                        QString::ConstIterator it = priceIntStr.constEnd() - 1;
                        for ( int i = 1; it != priceIntStr.constBegin() - 1; --it, ++i ) {
                            str = *it + str;
                            if ( i % 3 == 0 && i != priceIntStr.length() )
                                str = "," + str;
                        }
                        priceIntStr = str;
                    }

                    if ( buyOrders.contains(static_cast<quint64>(price)) ) {
                        m_buyersTableModel->setData(m_buyersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                    buyOrders[static_cast<quint64>(price)]);
                    }
                    else
                        m_buyersTableModel->setData(m_buyersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                    "");

                    m_buyersTableModel->setData(m_buyersTableModel->index(i, TBL_BUY_IND, QModelIndex()),
                                                it.value());
                    m_buyersTableModel->setData(m_buyersTableModel->index(i, TBL_PRICE_IND, QModelIndex()),
                                                priceIntStr);
                    if ( i == 0 ) {
                        firstQty = it.value();
                        firstPrice = it.key();
                    }
                }
                // Check
                if ( q.buyers.size() > 0 ) {
                    QMap<float, int>::Iterator it = q.buyers.end() - 1;
                    int qty = it.value();
                    float price = it.key();
                    if ( qty != firstQty || price != firstPrice ) {
                        int bad = 0;
                        (void)bad;
                    }
                }
                while ( m_buyersTableModel->rowCount() > maxRows ) {
                    m_buyersTableModel->removeRow( m_buyersTableModel->rowCount() - 1 );
                }
                Ui_ADMainWindow::sellersTableView->scrollToTop();
            }
            // Sellers
            {
                QMap<float, int>::Iterator it = q.sellers.end() - 1;
                int sellersSize = q.sellers.size();
                int lastQty = 0;
                float lastPrice = 0.0;
                for ( int i = 0; i < maxRows; ++i ) {
                    // View > Sellers
                    if ( maxRows > sellersSize ) {
                        if ( maxRows - sellersSize > i ) {
                            if ( m_sellersTableModel->rowCount() <= i )
                                m_sellersTableModel->insertRow(i, QModelIndex());
                            else {
                                m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                             "");
                                m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_SELL_IND, QModelIndex()),
                                                             "");
                                m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_PRICE_IND, QModelIndex()),
                                                             "");
                            }
                        }
                        else {
                            float price = it.key();
                            QString priceStr = QString("%1").arg(price);
                            int priceInt = static_cast<int>(price);
                            QString priceIntStr = QString("%1").arg(priceInt);
                            if ( priceInt < price )
                                priceIntStr = priceStr;
                            else {
                                QString str;
                                QString::ConstIterator it = priceIntStr.constEnd() - 1;
                                for ( int i = 1; it != priceIntStr.constBegin() - 1; --it, ++i ) {
                                    str = *it + str;
                                    if ( i % 3 == 0 && i != priceIntStr.length() )
                                        str = "," + str;
                                }
                                priceIntStr = str;
                            }

                            if ( m_sellersTableModel->rowCount() <= i )
                                m_sellersTableModel->insertRow(i, QModelIndex());

                            if ( sellOrders.contains(static_cast<quint64>(price)) ) {
                                m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                             sellOrders[static_cast<quint64>(price)]);
                            }
                            else
                                m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                             "");
                            m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_SELL_IND, QModelIndex()),
                                                         it.value());
                            m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_PRICE_IND, QModelIndex()),
                                                         priceIntStr);
                            lastQty = it.value();
                            lastPrice = it.key();
                            --it;
                        }
                    }
                    // View <= Sellers
                    else {
                        QMap<float, int>::Iterator thisIt = it - (sellersSize - maxRows) - i;

                        if ( m_sellersTableModel->rowCount() <= i )
                            m_sellersTableModel->insertRow(i, QModelIndex());

                        float price = thisIt.key();
                        QString priceStr = QString("%1").arg(price);
                        int priceInt = static_cast<int>(price);
                        QString priceIntStr = QString("%1").arg(priceInt);
                        if ( priceInt < price )
                            priceIntStr = priceStr;
                        else {
                            QString str;
                            QString::ConstIterator it = priceIntStr.constEnd() - 1;
                            for ( int i = 1; it != priceIntStr.constBegin() - 1; --it, ++i ) {
                                str = *it + str;
                                if ( i % 3 == 0 && i != priceIntStr.length() )
                                    str = "," + str;
                            }
                            priceIntStr = str;
                        }

                        if ( sellOrders.contains(static_cast<quint64>(price)) ) {
                            m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                         sellOrders[static_cast<quint64>(price)]);
                        }
                        else
                            m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_ORDER_IND, QModelIndex()),
                                                         "");
                        m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_SELL_IND, QModelIndex()),
                                                     thisIt.value());
                        m_sellersTableModel->setData(m_sellersTableModel->index(i, TBL_PRICE_IND, QModelIndex()),
                                                     priceIntStr);
                        lastQty = thisIt.value();
                        lastPrice = thisIt.key();
                    }
                }

                // Check
                {
                    QMap<float, int>::Iterator it = q.sellers.begin();
                    int qty = it.value();
                    float price = it.key();
                    if ( qty != lastQty || price != lastPrice ) {
                        int bad = 0;
                        (void)bad;
                    }
                }
                while ( m_sellersTableModel->rowCount() > maxRows ) {
                    m_sellersTableModel->removeRow( m_sellersTableModel->rowCount() - 1 );
                }

                Ui_ADMainWindow::sellersTableView->scrollToBottom();
            }
        }

        QMap<float, int>::Iterator it = q.sellers.begin();

        bool min_sellers = true;
        bool max_sellers = true;
        for ( int i = 0; it != q.sellers.end() && i < 3;
              ++i, ++it ) {
            int val = it.value();
            if ( val > 5 )
                min_sellers = false;
            if ( i == 0 && val < 15 )
                max_sellers = false;
        }

        bool min_buyers = true;
        bool max_buyers = true;
        for ( int i = 0; q.buyers.size() != 0 && it != q.buyers.begin() -1 && i < 3;
              ++i, --it ) {
            int val = it.value();
            if ( val > 5 )
                min_buyers = false;
            if ( i == 0 && val < 15 )
                max_buyers = false;
        }

        if ( min_sellers && max_buyers ) {
            qWarning("!!!!! MIN SELLERS, MAX BUYERS, now price =%.2f\n", q.lastPrice);
        }
        else if ( min_buyers && max_sellers ) {
            qWarning("!!!!! MIN BUYERS, MAX SELLERS, now price =%.2f\n", q.lastPrice);
        }
    }
}

void ADMainWindow::onHistoricalQuotesReceived ( ADConnection::Request req, QVector<ADConnection::HistoricalQuote> quotes )
{
    qWarning("historical quotes: %d", req.requestId());
    QVector<ADConnection::HistoricalQuote>::Iterator it = quotes.begin();
    for ( ; it != quotes.end(); ++it ) {
        ADConnection::HistoricalQuote& q = *it;
        qWarning("\tpaper=%d, open=%.2f, high=%.2f, low=%.2f, close=%.2f, volume=%.2f, dt=%s",
                 q.paperNo,
                 q.open,
                 q.high,
                 q.low,
                 q.close,
                 q.volume,
                 qPrintable(q.dt.toString("yyyy-MM-dd hh:mm:ss")));
    }
}

void ADMainWindow::onPositionChanged ( QString accCode,
                                       QString paperCode,
                                       int paperNo )
{
    (void)paperNo;

    if ( accCode != m_accCode || paperCode != m_papCode )
        return;

    ADConnection::Position pos;
    if ( ! m_adConnect.getPosition(accCode, paperCode, pos) )
        return;

    Ui_ADMainWindow::posLabel_2->setText(QString("[%1]").arg(pos.qty));
    Ui_ADMainWindow::varMarginLabel->setText(QString("%1").arg(pos.varMargin, 0, 'f', 1));
}

void ADMainWindow::onOrderStateChanged ( ADConnection::Order order,
                                         ADConnection::Order::State oldState,
                                         ADConnection::Order::State newState )
{
    (void)oldState;

    if ( order.getAccountCode() != m_accCode || order.getOrderPaperCode() != m_papCode )
        return;

    // Executed
    if ( newState == ADConnection::Order::ExecutedState ) {
        if ( order.getOrderType() == ADConnection::Order::Sell && -1 != m_sellOrders.indexOf(order) ) {
            m_sellOrders.removeAll(order);
            Ui_ADMainWindow::sellOrdersLabel->setText(m_sellOrders.size() == 0 ? "" : QString("%1").arg(m_sellOrders.size()));
        }
        else if ( order.getOrderType() == ADConnection::Order::Buy && -1 != m_buyOrders.indexOf(order) ) {
            m_buyOrders.removeAll(order);
            Ui_ADMainWindow::buyOrdersLabel->setText(m_buyOrders.size() == 0 ? "" : QString("%1").arg(m_buyOrders.size()));
        }
    }
    // Cancelled
    else if ( newState == ADConnection::Order::CancelledState  ) {
        if ( order.getOrderType() == ADConnection::Order::Sell && -1 != m_sellOrders.indexOf(order) ) {
            m_sellOrders.removeAll(order);
            Ui_ADMainWindow::sellOrdersLabel->setText(m_sellOrders.size() == 0 ? "" : QString("%1").arg(m_sellOrders.size()));
        }
        else if ( order.getOrderType() == ADConnection::Order::Buy && -1 != m_buyOrders.indexOf(order) ) {
            m_buyOrders.removeAll(order);
            Ui_ADMainWindow::buyOrdersLabel->setText(m_buyOrders.size() == 0 ? "" : QString("%1").arg(m_buyOrders.size()));
        }
    }
    // Accepted
    else if ( newState == ADConnection::Order::AcceptedState ) {
        if ( order.getOrderType() == ADConnection::Order::Sell && -1 == m_sellOrders.indexOf(order) ) {
            m_sellOrders.append(order);
            Ui_ADMainWindow::sellOrdersLabel->setText(m_sellOrders.size() == 0 ? "" : QString("%1").arg(m_sellOrders.size()));
        }
        else if ( order.getOrderType() == ADConnection::Order::Buy && -1 == m_buyOrders.indexOf(order) ) {
            m_buyOrders.append(order);
            Ui_ADMainWindow::buyOrdersLabel->setText(m_buyOrders.size() == 0 ? "" : QString("%1").arg(m_buyOrders.size()));
        }
    }

}

void ADMainWindow::onTrade ( ADConnection::Order order, quint32 qty )
{
    if ( order.getAccountCode() != m_accCode || order.getOrderPaperCode() != m_papCode )
        return;

    //XXX
    qWarning("order.state != Executed (%d), order.qty != qty (%d)",
             order.getOrderState() != ADConnection::Order::ExecutedState,
             order.getOrderQty() != qty);

    // Sell
    if ( order.getOrderType() == ADConnection::Order::Sell ) {
        m_pos -= qty;
    }
    // Buy
    else {
        m_pos += qty;
    }

    Ui_ADMainWindow::posLabel->setText(QString("%1").arg(m_pos));
}

void ADMainWindow::onOrderOperationResult ( ADConnection::Order,
                                            ADConnection::Order::Operation,
                                            ADConnection::Order::OperationResult )
{
}

void ADMainWindow::onEverySecond ()
{
    QDateTime srvDt;
    quint64 rxNet;
    quint64 txNet;
    quint64 rxDecoded;
    quint64 txEncoded;
    QString statusMsg;
    QString netStatMsg;
    if ( m_adConnect.serverTime(srvDt) && srvDt.isValid() &&
         m_adConnect.getNetworkStatistics(rxNet, txNet, rxDecoded, txEncoded) ) {
        statusMsg = srvDt.toString("dd.MM.yyyy hh:mm:ss");
        double rxDivisor = 1024.0;
        double txDivisor = 1024.0;
        QString rxMeasure( "kb" );
        QString txMeasure( "kb" );
        int rxAfterDot = 1;
        int txAfterDot = 1;
        if ( rxNet >= 1024*1024 ) {
            rxDivisor = 1024*1024;
            rxMeasure = "mb";
            rxAfterDot = 2;
        }
        if ( txNet >= 1024*1024 ) {
            txDivisor = 1024*1024;
            txMeasure = "mb";
            txAfterDot = 2;
        }

        netStatMsg = QString("[rx %2 %3, tx %4 %5]").
            arg((double)rxNet/rxDivisor, 0, 'f', rxAfterDot).
            arg(rxMeasure).
            arg((double)txNet/txDivisor, 0, 'f', txAfterDot).
            arg(txMeasure);
    }
    else {
        statusMsg = "Not connected!";
        netStatMsg = "";
    }
    statusBar()->showMessage(statusMsg);
    Ui_ADMainWindow::netStatLabel->setText(netStatMsg);
}

void ADMainWindow::closeEvent ( QCloseEvent* )
{
    qApp->exit();
}

void ADMainWindow::keyPressEvent ( QKeyEvent* ke )
{
    // Drop all orders
    if ( ke && ke->key() == Qt::Key_Escape ) {
        foreach ( ADConnection::Order order, m_sellOrders ) {
            m_adConnect.cancelOrder(order);
        }
        foreach ( ADConnection::Order order, m_buyOrders ) {
            m_adConnect.cancelOrder(order);
        }
    }
}

void ADMainWindow::keyPressEventADTableView (  ADTableView* view, QKeyEvent* ke )
{
    //
    // Selection change
    //
    if ( ke && ke->key() == Qt::Key_Up && view == Ui_ADMainWindow::buyersTableView &&
         Ui_ADMainWindow::buyersTableView->selectionModel()->hasSelection() ) {
        int row = 0;
        QModelIndex firstIdx = m_buyersTableModel->index( row, 0 );
        if ( Ui_ADMainWindow::buyersTableView->selectionModel()->isRowSelected(row, QModelIndex()) ) {
            Ui_ADMainWindow::buyersTableView->clearSelection();
            QModelIndex lastIdx = m_sellersTableModel->index( m_sellersTableModel->rowCount() - 1, 0 );
            Ui_ADMainWindow::sellersTableView->selectionModel()->
                setCurrentIndex(lastIdx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            Ui_ADMainWindow::sellersTableView->setFocus( Qt::OtherFocusReason );
        }
    }
    else if ( ke && ke->key() == Qt::Key_Down && view == Ui_ADMainWindow::sellersTableView &&
              Ui_ADMainWindow::sellersTableView->selectionModel()->hasSelection() ) {
        int row = m_sellersTableModel->rowCount() - 1;
        QModelIndex lastIdx = m_sellersTableModel->index( row, 0 );
        if ( Ui_ADMainWindow::sellersTableView->selectionModel()->isRowSelected(row, QModelIndex()) ) {
            Ui_ADMainWindow::sellersTableView->clearSelection();
            QModelIndex firstIdx = m_buyersTableModel->index( 0, 0 );
            Ui_ADMainWindow::buyersTableView->selectionModel()->
                setCurrentIndex(firstIdx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            Ui_ADMainWindow::buyersTableView->setFocus( Qt::OtherFocusReason );
        }
    }
    else if ( ke && ke->key() == Qt::Key_Escape ) {
        // Clear selection
        Ui_ADMainWindow::buyersTableView->selectionModel()->
            setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
        Ui_ADMainWindow::sellersTableView->selectionModel()->
            setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    }
}

void ADMainWindow::mousePressEventADTableView ( ADTableView* view, QMouseEvent* me )
{
    if ( view == 0 || me == 0 )
        return;

    //
    // Clear previously selected view
    //
    if ( view == Ui_ADMainWindow::buyersTableView )
        Ui_ADMainWindow::sellersTableView->selectionModel()->
            setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    else if ( view == Ui_ADMainWindow::sellersTableView )
        Ui_ADMainWindow::buyersTableView->clearSelection();

    //
    // Handle clicks and send orders
    //
    ADConnection::Order::Type orderType = ADConnection::Order::UnknownType;
    QModelIndex index;
    QStandardItemModel* model = 0;
    if ( view == Ui_ADMainWindow::buyersTableView ) {
        QModelIndex buyIndex = Ui_ADMainWindow::buyersTableView->indexAt(me->pos());
        if ( ! buyIndex.isValid() )
            return;
        index = buyIndex;
        orderType = ADConnection::Order::Buy;
        model = m_buyersTableModel;
    }
    else if ( view == Ui_ADMainWindow::sellersTableView ) {
        QModelIndex sellIndex = Ui_ADMainWindow::sellersTableView->indexAt(me->pos());
        if ( ! sellIndex.isValid() )
            return;
        index = sellIndex;
        orderType = ADConnection::Order::Sell;
        model = m_sellersTableModel;
    }
    else
        return;

    int row = index.row();
    if ( model->rowCount() <= row ) {
        qWarning("Row is big %d!", row);
        return;
    }
    QModelIndex priceInd = model->index(row, TBL_PRICE_IND);
    if ( ! priceInd.isValid() ) {
        qWarning("Item is not valid!");
        return;
    }
    QVariant data = priceInd.data();
    if ( ! data.isValid() || data.isNull() ) {
        qWarning("Price is null!");
        return;
    }

    QString priceStr = data.toString();
    priceStr.remove(QRegExp("[^\\d\\.]"));
    bool ok = false;
    double price = priceStr.toDouble(&ok);
    if ( ! ok ) {
        qWarning("Can't convert price to double!");
        return;
    }

    // If left -> try to buy/sell using this price
    if ( me->button() == Qt::LeftButton ) {
        // Reverse
        orderType = (orderType == ADConnection::Order::Buy ?
                     ADConnection::Order::Sell :
                     ADConnection::Order::Buy);
    }
    // If right -> try to send order better than this price
    else if ( me->button() == Qt::RightButton ) {
        if ( orderType == ADConnection::Order::Buy )
            price += Ui_ADMainWindow::betterSpinBox->value();
        else
            price -= Ui_ADMainWindow::betterSpinBox->value();
    }
    // If middle -> drop all orders
    else if ( me->button() == Qt::MidButton ) {
        foreach ( ADConnection::Order order, m_sellOrders ) {
            m_adConnect.cancelOrder(order);
        }
        foreach ( ADConnection::Order order, m_buyOrders ) {
            m_adConnect.cancelOrder(order);
        }
        return;
    }
    else {
        qWarning("Unknown mouse button! Nothing to do!");
        return;
    }

    //XXX: TODO
    quint32 qty = 1;

    ADConnection::Order order;
    ADConnection::Order::Operation op =
        m_adConnect.tradePaper( order, m_accCode, orderType,
                                m_papCode, qty, price );
}

/***************************************************************************************/