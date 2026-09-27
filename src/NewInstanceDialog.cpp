#include "NewInstanceDialog.h"
#include "Net.h"
#include "Paths.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
const QUrl kManifestUrl("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json");
enum Column { ColId, ColType, ColDate };
} // namespace

NewInstanceDialog::NewInstanceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Instance"));
    resize(460, 520);

    m_name = new QLineEdit(this);
    m_name->setMaxLength(64);
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Search versions..."));
    m_filter->setClearButtonEnabled(true);
    m_showSnapshots = new QCheckBox(tr("Snapshots"), this);
    m_showOld = new QCheckBox(tr("Old alpha/beta"), this);

    m_versions = new QTreeWidget(this);
    m_versions->setColumnCount(3);
    m_versions->setHeaderLabels({tr("Version"), tr("Type"), tr("Released")});
    m_versions->setRootIsDecorated(false);
    m_versions->header()->setSectionResizeMode(ColId, QHeaderView::Stretch);

    m_status = new QLabel(tr("Loading versions..."), this);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Create"));

    auto *form = new QFormLayout;
    form->addRow(tr("Name:"), m_name);

    auto *filters = new QHBoxLayout;
    filters->addWidget(m_filter, 1);
    filters->addWidget(m_showSnapshots);
    filters->addWidget(m_showOld);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(filters);
    layout->addWidget(m_versions, 1);
    layout->addWidget(m_status);
    layout->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_name, &QLineEdit::textEdited, this, [this]() {
        m_nameEdited = !m_name->text().isEmpty();
        validate();
    });
    connect(m_filter, &QLineEdit::textChanged, this, &NewInstanceDialog::populate);
    connect(m_showSnapshots, &QCheckBox::toggled, this, &NewInstanceDialog::populate);
    connect(m_showOld, &QCheckBox::toggled, this, &NewInstanceDialog::populate);
    connect(m_versions, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item) {
        if (item && !m_nameEdited)
            m_name->setText(item->text(ColId));
        validate();
    });
    connect(m_versions, &QTreeWidget::itemDoubleClicked, this, [this]() {
        if (m_buttons->button(QDialogButtonBox::Ok)->isEnabled())
            accept();
    });

    validate();
    loadManifest();
}

void NewInstanceDialog::loadManifest()
{
    const QString cache = Paths::data() + "/version_manifest_v2.json";
    Net::handle(Net::get(kManifestUrl), this, [this, cache](const Net::Result &r) {
        if (r.ok && r.json.contains("versions")) {
            QSaveFile f(cache);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(r.body);
                f.commit();
            }
            m_manifest = r.json["versions"].toArray();
            m_status->clear();
        } else {
            QFile f(cache);
            if (f.open(QIODevice::ReadOnly))
                m_manifest = QJsonDocument::fromJson(f.readAll()).object()["versions"].toArray();
            m_status->setText(m_manifest.isEmpty()
                                  ? tr("Could not load the version list: %1").arg(r.error)
                                  : tr("Offline: showing cached version list."));
        }
        populate();
    });
}

void NewInstanceDialog::populate()
{
    const QString selected = m_versions->currentItem() ? m_versions->currentItem()->text(ColId) : QString();
    const QString filter = m_filter->text().trimmed();
    m_versions->clear();

    for (const QJsonValue &v : std::as_const(m_manifest)) {
        const QJsonObject o = v.toObject();
        const QString type = o["type"].toString();
        if (type == "snapshot" && !m_showSnapshots->isChecked())
            continue;
        if ((type == "old_alpha" || type == "old_beta") && !m_showOld->isChecked())
            continue;
        const QString id = o["id"].toString();
        if (!filter.isEmpty() && !id.contains(filter, Qt::CaseInsensitive))
            continue;

        auto *item = new QTreeWidgetItem(m_versions);
        item->setText(ColId, id);
        item->setText(ColType, type);
        item->setText(ColDate, o["releaseTime"].toString().left(10));
        item->setData(ColId, Qt::UserRole, o["url"].toString());
        item->setData(ColId, Qt::UserRole + 1, o["sha1"].toString());
        if (id == selected)
            m_versions->setCurrentItem(item);
    }
    validate();
}

void NewInstanceDialog::validate()
{
    const QString name = m_name->text().trimmed();
    QString problem;
    if (!m_versions->currentItem())
        problem = tr("Select a version.");
    else if (!Instance::isValidName(name))
        problem = tr("Name may only contain letters, numbers, spaces, '.', '_' and '-'.");
    else if (QDir(Paths::instances() + '/' + name).exists())
        problem = tr("An instance with this name already exists.");

    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
    m_buttons->button(QDialogButtonBox::Ok)->setToolTip(problem);
}

Instance NewInstanceDialog::instance() const
{
    Instance inst;
    inst.name = m_name->text().trimmed();
    if (QTreeWidgetItem *item = m_versions->currentItem()) {
        inst.versionId = item->text(ColId);
        inst.versionUrl = item->data(ColId, Qt::UserRole).toString();
        inst.versionSha1 = item->data(ColId, Qt::UserRole + 1).toString();
    }
    return inst;
}
