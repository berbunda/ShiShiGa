#include "ServicePagesDialog.h"

#include "core/ServiceManager.h"
#include "services/ServiceRegistry.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>

ServicePagesDialog::ServicePagesDialog(ServiceManager *manager, QWidget *parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Service pages"));
    resize(600, 400);
    auto *layout = new QVBoxLayout(this);
    auto *hint = new QLabel(tr("Add your own AI service pages. Changes are saved immediately."), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    m_list = new QListWidget(this);
    layout->addWidget(m_list);
    auto *actions = new QHBoxLayout();
    auto *add = new QPushButton(tr("Add page..."), this);
    auto *edit = new QPushButton(tr("Edit..."), this);
    auto *remove = new QPushButton(tr("Remove"), this);
    auto *open = new QPushButton(tr("Open"), this);
    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(remove);
    actions->addStretch();
    actions->addWidget(open);
    layout->addLayout(actions);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(add, &QPushButton::clicked, this, &ServicePagesDialog::addPage);
    connect(edit, &QPushButton::clicked, this, [this] { editPage(selectedId()); });
    connect(remove, &QPushButton::clicked, this, &ServicePagesDialog::removePage);
    connect(open, &QPushButton::clicked, this, [this] {
        if (m_manager->openService(selectedId()))
            accept();
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this] { editPage(selectedId()); });
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this, edit, remove, open] {
        const bool selected = !selectedId().isEmpty();
        edit->setEnabled(selected);
        remove->setEnabled(selected);
        open->setEnabled(selected);
    });
    connect(manager, &ServiceManager::pagesChanged, this, &ServicePagesDialog::refresh);
    edit->setEnabled(false);
    remove->setEnabled(false);
    open->setEnabled(false);
    refresh();
}

QString ServicePagesDialog::selectedId() const
{
    const auto *item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void ServicePagesDialog::refresh()
{
    const QString selected = selectedId();
    m_list->clear();
    for (const auto &page : ServiceRegistry::catalog()) {
        auto *item = new QListWidgetItem(page.displayName + QStringLiteral(" — ")
                                            + page.startUrl.toDisplayString(), m_list);
        item->setData(Qt::UserRole, page.id);
        if (page.id == selected)
            m_list->setCurrentItem(item);
    }
}

void ServicePagesDialog::addPage()
{
    editPage(QString());
}

void ServicePagesDialog::editPage(const QString &id)
{
    auto page = ServiceRegistry::entryFor(id);
    const bool adding = id.isEmpty();
    if (!adding && page.id.isEmpty())
        return;
    QDialog editor(this);
    editor.setWindowTitle(adding ? tr("Add service page") : tr("Edit service page"));
    editor.resize(500, 180);
    auto *layout = new QFormLayout(&editor);
    auto *name = new QLineEdit(page.displayName, &editor);
    auto *url = new QLineEdit(page.startUrl.toString(), &editor);
    name->setPlaceholderText(tr("Service name"));
    url->setPlaceholderText(QStringLiteral("https://"));
    layout->addRow(tr("Name:"), name);
    layout->addRow(tr("Page URL:"), url);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &editor);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &editor, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &editor, [&] {
        if (adding && page.id.isEmpty())
            page.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        page.profileId = page.id;
        page.displayName = name->text().trimmed();
        page.startUrl = QUrl(url->text().trimmed(), QUrl::StrictMode);
        page.enabled = true;
        if (!ServiceRegistry::isValidPage(page)) {
            QMessageBox::warning(&editor, tr("Invalid page"),
                tr("Enter a name and a full HTTP or HTTPS URL with a host and without credentials."));
            return;
        }
        QString error;
        if (!m_manager->savePage(page, &error)) {
            QMessageBox::warning(&editor, tr("Could not save page"), error);
            return;
        }
        editor.accept();
    });
    editor.exec();
}

void ServicePagesDialog::removePage()
{
    const QString id = selectedId();
    if (id.isEmpty())
        return;
    if (QMessageBox::question(this, tr("Remove page"),
        tr("Remove this page from the list? Its saved browser profile will be kept.")) != QMessageBox::Yes)
        return;
    QString error;
    if (!m_manager->removePage(id, &error))
        QMessageBox::warning(this, tr("Could not remove page"), error);
}
