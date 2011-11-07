#include "ADTableView.h"
#include "ADMainWindow.h"

/***************************************************************************************/

ADTableView::ADTableView ( QWidget* parent ) :
    QTableView(parent),
    m_adMainWindow(0)
{}

void ADTableView::setADMainWindow ( ADMainWindow* wnd )
{
    m_adMainWindow = wnd;
}

void ADTableView::mousePressEvent ( QMouseEvent* me )
{
    if ( me && m_adMainWindow )
        m_adMainWindow->mousePressEventADTableView(this, me);
    QTableView::mousePressEvent( me );
}

void ADTableView::keyPressEvent ( QKeyEvent* ke )
{
    if ( ke && m_adMainWindow )
        m_adMainWindow->keyPressEventADTableView(this, ke);
    QTableView::keyPressEvent( ke );
}

/***************************************************************************************/
