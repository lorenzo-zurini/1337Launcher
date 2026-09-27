#pragma once

#include "Account.h"

#include <QDialog>

class MicrosoftAuth;
class QLabel;
class QPushButton;

// Shows the Microsoft device code and waits for the user to finish signing in.
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    Account account() const { return m_account; }
    int exec() override;

private:
    MicrosoftAuth *m_auth;
    QLabel *m_instructions;
    QLabel *m_code;
    QLabel *m_status;
    QPushButton *m_openButton;
    QPushButton *m_copyButton;
    QString m_url;
    Account m_account;
};
