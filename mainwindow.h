#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <QJsonDocument>
#include <QStringList>
#include <QString>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTreeView>
#include <QDir>
#include <QSslSocket>
#include <QtConcurrent/QtConcurrentRun>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void GetVersionManifest();
    QDir DataDir;
    QDir InstancesDir;

signals:
    void onReady();

private slots:
    void on_AddInstanceButton_clicked();
    void onVersionManifestGet(QNetworkReply *reply);
    void onVersionJsonGet(QNetworkReply *reply);
    void FileDownloadLoop(...);
    void on_CreateInstanceButton_clicked();
    void on_CancelInstanceButton_clicked();
    void on_InstanceNameInput_textEdited(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *VersionManifestGetManager;
    QNetworkAccessManager *VersionJsonGetManager;
    QNetworkAccessManager *FileGetManager;
};
#endif // MAINWINDOW_H
