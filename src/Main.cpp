#include <QApplication>
#include <ADConnection.h>

#include "ADMainWindow.h"

/*****************************************************************************/

int main ( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    //XXX: TODO
    QString login = "";
    QString passwd = "";
    QString accCode = "";
    QString papCode = "";

    ADMainWindow mainWindow(login, passwd, accCode, papCode);
    mainWindow.show();
    return app.exec();
}

/*****************************************************************************/
