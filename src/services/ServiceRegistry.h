#pragma once

#include <QIcon>
#include <QList>
#include <QString>
#include <QUrl>

struct ServiceCatalogEntry
{
    QString id;
    QString displayName;
    QUrl startUrl;
    QString profileId;
    QUrl faviconUrl;
    QString faviconResourcePath;
    bool enabled = false;
};

class ServiceRegistry
{
public:
    static QList<ServiceCatalogEntry> catalog();
    static QList<ServiceCatalogEntry> enabledServices();
    static ServiceCatalogEntry entryFor(const QString &serviceId);
    static bool isEnabled(const QString &serviceId);
    static bool isValidPage(const ServiceCatalogEntry &entry);
    static bool saveCatalog(const QList<ServiceCatalogEntry> &entries, QString *errorMessage = nullptr);
    static QIcon faviconFor(const ServiceCatalogEntry &entry, int logicalSize = 40);
};
