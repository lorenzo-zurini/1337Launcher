#include "SettingsDialog.h"
#include "Paths.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(480);
    QSettings &s = Paths::settings();

    m_javaPath = new QLineEdit(s.value("java/path").toString(), this);
    m_javaPath->setPlaceholderText(tr("Automatic (download the right Java for each version)"));
    auto *browse = new QPushButton(tr("Browse..."), this);
    auto *javaRow = new QHBoxLayout;
    javaRow->addWidget(m_javaPath, 1);
    javaRow->addWidget(browse);

    m_minMemory = new QSpinBox(this);
    m_minMemory->setRange(128, 65536);
    m_minMemory->setSingleStep(256);
    m_minMemory->setSuffix(" MiB");
    m_minMemory->setValue(s.value("java/minMemory", 512).toInt());

    m_maxMemory = new QSpinBox(this);
    m_maxMemory->setRange(512, 65536);
    m_maxMemory->setSingleStep(512);
    m_maxMemory->setSuffix(" MiB");
    m_maxMemory->setValue(s.value("java/maxMemory", 4096).toInt());

    m_extraArgs = new QLineEdit(s.value("java/extraArgs").toString(), this);
    m_extraArgs->setPlaceholderText("-XX:+UseG1GC ...");

    auto *dataDir = new QPushButton(tr("Open data folder"), this);

    auto *form = new QFormLayout;
    form->addRow(tr("Java executable:"), javaRow);
    form->addRow(tr("Minimum memory:"), m_minMemory);
    form->addRow(tr("Maximum memory:"), m_maxMemory);
    form->addRow(tr("Extra JVM arguments:"), m_extraArgs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(dataDir, 0, Qt::AlignLeft);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, tr("Select Java executable"));
        if (!path.isEmpty())
            m_javaPath->setText(path);
    });
    connect(dataDir, &QPushButton::clicked, this,
            []() { QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::data())); });
    connect(m_minMemory, &QSpinBox::valueChanged, this, [this](int v) {
        if (m_maxMemory->value() < v)
            m_maxMemory->setValue(v);
    });
    connect(m_maxMemory, &QSpinBox::valueChanged, this, [this](int v) {
        if (m_minMemory->value() > v)
            m_minMemory->setValue(v);
    });
}

void SettingsDialog::accept()
{
    QSettings &s = Paths::settings();
    s.setValue("java/path", m_javaPath->text().trimmed());
    s.setValue("java/minMemory", m_minMemory->value());
    s.setValue("java/maxMemory", m_maxMemory->value());
    s.setValue("java/extraArgs", m_extraArgs->text().trimmed());
    s.sync();
    QDialog::accept();
}
