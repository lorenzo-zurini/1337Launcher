#include "LoginDialog.h"
#include "MicrosoftAuth.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_auth(new MicrosoftAuth(this))
{
    setWindowTitle(tr("Microsoft Login"));
    setMinimumWidth(420);

    m_instructions = new QLabel(tr("Contacting Microsoft..."), this);
    m_instructions->setWordWrap(true);

    m_code = new QLabel(this);
    QFont f = m_code->font();
    f.setPointSize(f.pointSize() * 2);
    f.setBold(true);
    f.setFamily("monospace");
    m_code->setFont(f);
    m_code->setAlignment(Qt::AlignCenter);
    m_code->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_openButton = new QPushButton(tr("Open browser"), this);
    m_copyButton = new QPushButton(tr("Copy code"), this);
    m_openButton->setEnabled(false);
    m_copyButton->setEnabled(false);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);

    auto *row = new QHBoxLayout;
    row->addWidget(m_openButton);
    row->addWidget(m_copyButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_instructions);
    layout->addWidget(m_code);
    layout->addLayout(row);
    layout->addWidget(m_status);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, [this]() {
        m_auth->cancel();
        reject();
    });
    connect(m_openButton, &QPushButton::clicked, this, [this]() { QDesktopServices::openUrl(QUrl(m_url)); });
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_code->text());
        m_status->setText(tr("Code copied to clipboard."));
    });

    connect(m_auth, &MicrosoftAuth::deviceCodeReady, this, [this](const QString &code, const QString &uri) {
        m_url = uri;
        m_code->setText(code);
        m_instructions->setText(
            tr("To sign in, open <a href=\"%1\">%1</a> in your browser and enter this code:").arg(uri.toHtmlEscaped()));
        m_instructions->setOpenExternalLinks(true);
        m_openButton->setEnabled(true);
        m_copyButton->setEnabled(true);
        QApplication::clipboard()->setText(code);
    });
    connect(m_auth, &MicrosoftAuth::status, m_status, &QLabel::setText);
    connect(m_auth, &MicrosoftAuth::succeeded, this, [this](const Account &account) {
        m_account = account;
        accept();
    });
    connect(m_auth, &MicrosoftAuth::failed, this, [this](const QString &error) {
        QMessageBox::warning(this, tr("Login failed"), error);
        reject();
    });
}

int LoginDialog::exec()
{
    QMetaObject::invokeMethod(m_auth, &MicrosoftAuth::startDeviceLogin, Qt::QueuedConnection);
    return QDialog::exec();
}
