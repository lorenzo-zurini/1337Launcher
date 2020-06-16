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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void getData(const QString, const QString);
    void GetVersionManifest();
    void AddAllVersions();
    void GetVersionJson(const QString);
    QDir DataDir;
    QDir InstancesDir;

signals:
    void onReady();

private slots:
    void on_AddInstanceButton_clicked();
    void onVersionManifestGet(QNetworkReply *reply);
    void onVersionJsonGet(QNetworkReply *reply);
    void on_CreateInstanceButton_clicked();
    void UpdateInstancesTable();

    void on_ExitButton_clicked();

    void on_CancelInstanceButton_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *VersionManifestGetManager;
    QNetworkAccessManager *VersionJsonGetManager;
    void InstanceWatch();
};
#endif // MAINWINDOW_H
