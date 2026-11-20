#include "ServiceRegistry.h"

#include "core/ServiceFaviconProvider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>

namespace {
QString catalogPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(QStringLiteral("ShiShiGa/services.json"));
}
} // namespace

bool ServiceRegistry::isValidPage(const ServiceCatalogEntry &entry)
{
    static const QRegularExpression safeId(QStringLiteral("^[a-z0-9_-]+$"));
    const QUrl &url = entry.startUrl;
    return safeId.match(entry.id).hasMatch()
        && entry.profileId == entry.id
        && !entry.displayName.trimmed().isEmpty()
        && url.isValid() && !url.host().isEmpty()
        && (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"))
        && url.userInfo().isEmpty();
}

QList<ServiceCatalogEntry> ServiceRegistry::catalog()
{
    QFile file(catalogPath());
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QList<ServiceCatalogEntry> entries;
    QSet<QString> ids;
    for (const QJsonValue &value : document.array()) {
        const QJsonObject object = value.toObject();
        ServiceCatalogEntry entry;
        entry.id = object.value(QStringLiteral("id")).toString();
        entry.displayName = object.value(QStringLiteral("name")).toString();
        entry.startUrl = QUrl(object.value(QStringLiteral("url")).toString());
        entry.profileId = entry.id;
        entry.enabled = true;
        if (isValidPage(entry) && !ids.contains(entry.id)) {
            ids.insert(entry.id);
            entries.append(entry);
        }
    }
    return entries;
}

bool ServiceRegistry::saveCatalog(const QList<ServiceCatalogEntry> &entries, QString *errorMessage)
{
    QJsonArray array;
    QSet<QString> ids;
    for (const auto &entry : entries) {
        if (!isValidPage(entry) || ids.contains(entry.id)) {
            if (errorMessage)
                *errorMessage = QObject::tr("Each page needs a name, a unique ID and an HTTP(S) URL without credentials.");
            return false;
        }
        ids.insert(entry.id);
        array.append(QJsonObject{{QStringLiteral("id"), entry.id},
                                 {QStringLiteral("name"), entry.displayName.trimmed()},
                                 {QStringLiteral("url"), entry.startUrl.toString(QUrl::FullyEncoded)}});
    }
    const QString path = catalogPath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not create the pages directory.");
        return false;
    }
    QSaveFile file(path);
    const QByteArray data = QJsonDocument(array).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    return true;
}

QList<ServiceCatalogEntry> ServiceRegistry::enabledServices()
{
    return catalog();
}

ServiceCatalogEntry ServiceRegistry::entryFor(const QString &serviceId)
{
    for (const auto &entry : catalog()) {
        if (entry.id == serviceId)
            return entry;
    }
    return {};
}

bool ServiceRegistry::isEnabled(const QString &serviceId)
{
    return !entryFor(serviceId).id.isEmpty();
}

QIcon ServiceRegistry::faviconFor(const ServiceCatalogEntry &entry, int logicalSize)
{
    if (!entry.faviconResourcePath.isEmpty()) {
        const QIcon resourceIcon(entry.faviconResourcePath);
        if (!resourceIcon.isNull())
            return ServiceFaviconProvider::normalizedIcon(resourceIcon, logicalSize);
    }

    return ServiceFaviconProvider::placeholderIcon(entry.displayName, logicalSize);
}
