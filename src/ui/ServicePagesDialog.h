#pragma once

#include <QDialog>

class QListWidget;
class ServiceManager;

class ServicePagesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServicePagesDialog(ServiceManager *manager, QWidget *parent = nullptr);
    void addPage();

private:
    void refresh();
    void editPage(const QString &id);
    void removePage();
    QString selectedId() const;

    ServiceManager *m_manager = nullptr;
    QListWidget *m_list = nullptr;
};
