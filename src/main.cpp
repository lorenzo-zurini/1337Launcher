#include "Account.h"
#include "MainWindow.h"
#include "Paths.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("1337Launcher");
    QApplication::setApplicationVersion(LAUNCHER_VERSION);
    QApplication::setDesktopFileName("1337launcher");
    QApplication::setWindowIcon(QIcon(":/icon"));

    Paths::init();
    AccountStore::instance().load();

    MainWindow window;
    window.show();
    return app.exec();
}
