#include <QApplication>
#include <ADConnection.h>

#include "ADMainWindow.h"

/*****************************************************************************/

int main ( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    ADMainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}

/*****************************************************************************/
