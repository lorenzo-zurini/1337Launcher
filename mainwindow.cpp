#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("1337Launcher by Lewegee");

    DataDir.setPath(QCoreApplication::applicationDirPath());
    DataDir.cdUp();
    DataDir.cdUp();
    DataDir.setPath(DataDir.path() + "/Data");
    DataDir.mkpath(DataDir.path());

    InstancesDir.setPath(DataDir.path() + "/Instances");
    InstancesDir.mkpath(InstancesDir.path());

    VersionManifestGetManager = new QNetworkAccessManager();
    connect(VersionManifestGetManager, &QNetworkAccessManager::finished, this, &MainWindow::onVersionManifestGet);
    VersionJsonGetManager = new QNetworkAccessManager();
    connect(VersionJsonGetManager, &QNetworkAccessManager::finished, this, &MainWindow::onVersionJsonGet);

    MainWindow::GetVersionManifest();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateInstancesTable()
{

}

void MainWindow::on_AddInstanceButton_clicked()
{
    ui->InstancesTable->hide();
    ui->AddInstanceButton->setEnabled(0);
    ui->CancelInstanceButton->setEnabled(1);
}

void MainWindow::on_CancelInstanceButton_clicked()
{
    ui->InstancesTable->show();
    ui->AddInstanceButton->setEnabled(1);
    ui->CancelInstanceButton->setEnabled(0);
}

void MainWindow::GetVersionManifest()
{
    QUrl url;
    url.setUrl("https://launchermeta.mojang.com/mc/game/version_manifest.json");
    QNetworkRequest request;
    request.setUrl(url);
    VersionManifestGetManager->get(request);
}

void MainWindow::onVersionManifestGet(QNetworkReply *reply)
{
    if(reply->error())
        {
        qDebug() << reply->errorString();
        }
    else
        {
        QFile VersionManifest(DataDir.path() + "/" + "VersionManifest.json");
        qDebug() << DataDir.path() + "/" + "VersionManifest.json";

        if(VersionManifest.open(QIODevice::WriteOnly | QIODevice::Text))
            {
            VersionManifest.write(reply->readAll());
            VersionManifest.close();
            }

        MainWindow::AddAllVersions();
        }
}

void MainWindow::AddAllVersions()
{
        QFile VersionManifest(DataDir.path() + "/" + "VersionManifest.json");
        VersionManifest.open(QIODevice::ReadOnly | QIODevice::Text);
        QJsonDocument JsonDocument = QJsonDocument::fromJson(VersionManifest.readAll());
        QJsonObject RootObject = JsonDocument.object();
        QJsonArray JsonArray = RootObject["versions"].toArray();

        ui->AvailableVersionsTreeWidget->clear();

        QJsonObject CurrentObject;
        QTreeWidgetItem *ReleasesTab = new QTreeWidgetItem(ui->AvailableVersionsTreeWidget);
        ReleasesTab->setText(0, "Releases");
        QTreeWidgetItem *SnapshotsTab = new QTreeWidgetItem(ui->AvailableVersionsTreeWidget);
        SnapshotsTab->setText(0, "Snapshots");
        QTreeWidgetItem *OldBetasTab = new QTreeWidgetItem(ui->AvailableVersionsTreeWidget);
        OldBetasTab->setText(0, "Old Betas");
        QTreeWidgetItem *OldAlphasTab = new QTreeWidgetItem(ui->AvailableVersionsTreeWidget);
        OldAlphasTab->setText(0, "Old Alphas");
        for (int i = 0;i <= JsonArray.size(); i++)
        {
            CurrentObject = JsonArray.at(i).toObject();
            if(CurrentObject["type"].toString() == "snapshot")
            {
                QTreeWidgetItem *NewItem = new QTreeWidgetItem();
                NewItem->setText(0, CurrentObject["id"].toString());
                NewItem->setText(1, CurrentObject["url"].toString());
                SnapshotsTab->addChild(NewItem);
            }
            else if (CurrentObject["type"].toString() == "release")
            {
                QTreeWidgetItem *NewItem = new QTreeWidgetItem();
                NewItem->setText(0, CurrentObject["id"].toString());
                NewItem->setText(1, CurrentObject["url"].toString());
                ReleasesTab->addChild(NewItem);
            }
            else if (CurrentObject["type"].toString() == "old_beta")
            {
                QTreeWidgetItem *NewItem = new QTreeWidgetItem();
                NewItem->setText(0, CurrentObject["id"].toString());
                NewItem->setText(1, CurrentObject["url"].toString());
                OldBetasTab->addChild(NewItem);
            }
            else if (CurrentObject["type"].toString() == "old_alpha")
            {
                QTreeWidgetItem *NewItem = new QTreeWidgetItem();
                NewItem->setText(0, CurrentObject["id"].toString());
                NewItem->setText(1, CurrentObject["url"].toString());
                OldAlphasTab->addChild(NewItem);
            }
        }

}

void MainWindow::on_CreateInstanceButton_clicked()
{
    QList<QTreeWidgetItem *> SelectionList;
    SelectionList = ui->AvailableVersionsTreeWidget->selectedItems();
    QString URLString = SelectionList.first()->text(1);

    QString InstanceName = ui->InstanceNameInput->text();
    GetVersionJson(URLString);
}

void MainWindow::GetVersionJson(const QString URLString)
{
    QUrl url;
    url.setUrl(URLString);
    QNetworkRequest request;
    request.setUrl(url);
    VersionJsonGetManager->get(request);
}

void MainWindow::onVersionJsonGet(QNetworkReply *reply)
{
    if(reply->error())
        {
        qDebug() << reply->errorString();
        }
    else
        {
        //QString FileName = reply->url().fileName();
        QString InstanceName = ui->InstanceNameInput->text();
        QDir NewInstanceDirectory;
        NewInstanceDirectory.setPath(InstancesDir.path() + "/" + InstanceName);
        NewInstanceDirectory.mkpath(NewInstanceDirectory.path());
        QFile VersionJson(NewInstanceDirectory.path() + "/" + reply->url().fileName());
        qDebug() << NewInstanceDirectory.path() + "/" + reply->url().fileName();

        if(VersionJson.open(QIODevice::WriteOnly | QIODevice::Text))
            {
            VersionJson.write(reply->readAll());
            VersionJson.close();
            }
        }
}

void MainWindow::on_ExitButton_clicked()
    {
    qApp->exit();
    }
