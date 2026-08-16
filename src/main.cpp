#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Weqi"));
    app.setApplicationDisplayName(QStringLiteral("Weqi"));
    app.setOrganizationName(QStringLiteral("Weqi"));

    MainWindow window;
    window.show();

    return app.exec();
}
