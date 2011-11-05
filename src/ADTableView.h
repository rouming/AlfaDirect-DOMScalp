#ifndef ADTABLEVIEW_H
#define ADTABLEVIEW_H

#include <QTableView>

class ADMainWindow;

class ADTableView: public QTableView
{
public:
    ADTableView ( QWidget* parent = 0 );

    void setADMainWindow ( ADMainWindow* );

private:
    void mousePressEvent ( QMouseEvent* );
    void keyPressEvent ( QKeyEvent* );

private:
    ADMainWindow* m_adMainWindow;
};


#endif // ADTABLEVIEW_H
