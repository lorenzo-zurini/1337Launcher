#pragma once

#include "Instance.h"

#include <QDialog>
#include <QJsonArray>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTreeWidget;

class NewInstanceDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewInstanceDialog(QWidget *parent = nullptr);

    Instance instance() const;

private:
    void loadManifest();
    void populate();
    void validate();

    QLineEdit *m_name;
    QLineEdit *m_filter;
    QCheckBox *m_showSnapshots;
    QCheckBox *m_showOld;
    QTreeWidget *m_versions;
    QLabel *m_status;
    QDialogButtonBox *m_buttons;
    QJsonArray m_manifest;
    bool m_nameEdited = false;
};
