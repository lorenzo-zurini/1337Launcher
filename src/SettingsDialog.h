#pragma once

#include <QDialog>

class QLineEdit;
class QSpinBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    void accept() override;

private:
    QLineEdit *m_javaPath;
    QSpinBox *m_minMemory;
    QSpinBox *m_maxMemory;
    QLineEdit *m_extraArgs;
    QLineEdit *m_clientId;
};
