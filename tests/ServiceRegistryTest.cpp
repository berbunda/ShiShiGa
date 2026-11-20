#include "services/ServiceRegistry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(QStringLiteral("ShiShiGa/services.json"));
    // Refuse to overwrite a pre-existing test catalog.
    if (QFile::exists(path)) {
        qCritical("Test catalog already exists; refusing to overwrite it.");
        return 1;
    }
    int failures = 0;
    const auto check = [&failures](bool condition, const char *message) {
        if (!condition) {
            qCritical("%s", message);
            ++failures;
        }
    };
    check(ServiceRegistry::catalog().isEmpty(), "Fresh installation must have no predefined pages");
    ServiceCatalogEntry first;
    first.id = QStringLiteral("test-page-1");
    first.profileId = first.id;
    first.displayName = QString::fromUtf8("Мой сервис");
    first.startUrl = QUrl(QStringLiteral("https://example.com/chat?mode=ai#start"));
    first.enabled = true;
    auto second = first;
    second.id = second.profileId = QStringLiteral("test-page-2");
    QString error;
    check(ServiceRegistry::saveCatalog({first, second}, &error), "Saving two pages must succeed");
    auto pages = ServiceRegistry::catalog();
    check(pages.size() == 2 && pages.first().displayName == first.displayName
          && pages.first().startUrl == first.startUrl && pages.last().profileId != first.profileId,
          "Round trip must preserve Unicode, full URL and separate profiles for the same site");
    auto invalid = first;
    for (const auto &url : {"file:///tmp/page.html", "javascript:alert(1)", "https://", "https://user:pass@example.com"}) {
        invalid.startUrl = QUrl(QString::fromLatin1(url));
        check(!ServiceRegistry::saveCatalog({invalid}, &error), "Invalid URL must be rejected");
    }
    invalid = first;
    invalid.id = invalid.profileId = QStringLiteral("../escape");
    check(!ServiceRegistry::saveCatalog({invalid}, &error), "Unsafe profile IDs must be rejected");
    invalid = first;
    invalid.displayName = QStringLiteral("   ");
    check(!ServiceRegistry::saveCatalog({invalid}, &error), "Blank names must be rejected");
    check(!ServiceRegistry::saveCatalog({first, first}, &error), "Duplicate IDs must be rejected");
    check(ServiceRegistry::catalog().size() == 2, "Rejected edits must leave the saved catalog intact");
    first.displayName = QStringLiteral("Renamed");
    first.startUrl = QUrl(QStringLiteral("http://localhost:8080/ai"));
    check(ServiceRegistry::saveCatalog({first}, &error), "Editing and removing a page must succeed");
    check(ServiceRegistry::entryFor(first.id).displayName == first.displayName
          && ServiceRegistry::entryFor(second.id).id.isEmpty(), "Edited page must persist and removed page must be absent");
    check(ServiceRegistry::saveCatalog({}, &error) && ServiceRegistry::catalog().isEmpty(),
          "Removing the final page must persist an empty catalog");
    check(QFile::remove(path), "Test catalog cleanup must succeed");
    return failures == 0 ? 0 : 1;
}
