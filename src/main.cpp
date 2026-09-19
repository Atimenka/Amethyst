#include <QApplication>
#include <QCoreApplication>
#include <QImageReader>
#include <QDirIterator>
#include <QDir>
#include <QDebug>
#include <QResource>
#include <QFontDatabase>

#include "app/WelcomeWindow/WelcomeForm/welcome_form.h"
#include "core/locale/LanguageManager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Atimenka");
    QCoreApplication::setApplicationName("Amethyst");

#if !defined(Q_OS_MAC)
    QIcon appIcon(":/icons/icon.svg");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/amethyst.ico");
    }
    a.setWindowIcon(appIcon);
#endif

    // - - Fonts - -
    LanguageManager::instance().loadUserDefaultLocale();

    int jbFontRegId = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
    int jbFontBoldId = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Bold.ttf");
    int jbFontItalId = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Italic.ttf");

    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(jbFontRegId);
    QString jbFontFamily = !fontFamilies.isEmpty() ? fontFamilies.at(0) : "Sans Serif";

    qDebug() << jbFontFamily;

    // - - Themes - -

    // Icons
    Q_INIT_RESOURCE(phoicons);
    QIcon::setThemeSearchPaths({":/icons"});
    QIcon::setThemeName("phoicons");         // маленькими буквами!

    qDebug() << "=== SYSTEM DEBUG ===";
    qDebug() << "Supported formats:" << QImageReader::supportedImageFormats();
    qDebug() << "=== THEME DEBUG ===";
    qDebug() << "Theme Search Paths:" << QIcon::themeSearchPaths();
    qDebug() << "Current Theme Name:" << QIcon::themeName();

    // Переименовал в checkTheme, чтобы не было конфликта
    QFile checkTheme(":/icons/phoicons/index.theme");
    qDebug() << "index.theme exists in resources:" << checkTheme.exists();

    qDebug() << "=== RESOURCE TREE ===";
    QDirIterator it(":/icons", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << "Found resource:" << it.next();
    }
    qDebug() << "====================";

    // Style
    QApplication::setStyle("Fusion");

    QFile baseStyleFile(":/styles/base.qss");
    if (!baseStyleFile.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open the baseStyle file: " << baseStyleFile.errorString();
        return 1;
    }

    // Переименовал в qssThemeFile
    QFile qssThemeFile(":/styles/dark.qss");
    if (!qssThemeFile.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open the theme file: " << qssThemeFile.errorString();
        return 1;
    }

    QString baseStyle = QLatin1String(baseStyleFile.readAll());
    QString themeData = QLatin1String(qssThemeFile.readAll());

    baseStyleFile.close();
    qssThemeFile.close(); // Теперь закрываем правильный файл

    a.setStyleSheet(baseStyle + "\n" + themeData);

    WelcomeForm wf;

    //cli path selector
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        const QString projectPath = args.at(1);
        if (QDir(projectPath).exists()) {
            wf.openProject(projectPath);
        } else {
            qWarning() << "Project path does not exist:" << projectPath;
            wf.show();
        }
    } else {
        wf.show();
    }
    return QCoreApplication::exec();
}
