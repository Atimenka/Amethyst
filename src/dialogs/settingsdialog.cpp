/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "settingsdialog.h"

#include "core/settings/appsettings.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "core/locale/LanguageManager.h"

static QString resolvedExecutable(const QString &userPath, const QString &exeName)
{
    if (!userPath.trimmed().isEmpty())
        return userPath.trimmed();
    return QStandardPaths::findExecutable(exeName);
}

static bool isRunnableExecutable(const QString &path)
{
    if (path.trimmed().isEmpty())
        return false;
    const QFileInfo fi(path.trimmed());
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

static void setStatusLabel(QLabel *lbl, bool ok, const QString &text)
{
    lbl->setText(ok ? QStringLiteral("✓ ") + text : QStringLiteral("✗ ") + text);
    lbl->setProperty("statusState", ok ? "ok" : "missing");
    lbl->style()->unpolish(lbl);
    lbl->style()->polish(lbl);
    lbl->update();
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName("settingsDialog");
    setWindowTitle(tr("Настройки Amethyst"));
    setModal(true);
    setMinimumSize(780, 560);
    setSizeGripEnabled(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("settingsTabWidget");

    // ==========================================
    // TAB 1: Дизассемблер (Disassembler)
    // ==========================================
    auto *disasmTab = new QWidget();
    auto *disasmScroll = new QScrollArea();
    disasmScroll->setWidgetResizable(true);
    disasmScroll->setFrameShape(QFrame::NoFrame);

    auto *disasmContent = new QWidget();
    auto *disasmLayout = new QVBoxLayout(disasmContent);
    disasmLayout->setContentsMargins(12, 12, 12, 12);
    disasmLayout->setSpacing(14);

    // Backend group
    auto *backendGroup = new QGroupBox(tr("Основной движок дизассемблера"), disasmContent);
    auto *backendForm = new QFormLayout(backendGroup);
    backendForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    backendForm->setSpacing(10);

    m_backendCombo = new QComboBox(backendGroup);
    m_backendCombo->addItem(tr("objdump (GNU Binutils)"), static_cast<int>(AppSettings::DisasmBackend::Objdump));
    m_backendCombo->addItem(tr("radare2 (r2)"), static_cast<int>(AppSettings::DisasmBackend::Radare2));
    backendForm->addRow(tr("Бэкенд:"), m_backendCombo);

    m_syntaxCombo = new QComboBox(backendGroup);
    m_syntaxCombo->addItem(tr("Intel (рекомендуется)"), static_cast<int>(AppSettings::AsmSyntax::Intel));
    m_syntaxCombo->addItem(tr("AT&T"), static_cast<int>(AppSettings::AsmSyntax::Att));
    backendForm->addRow(tr("Синтаксис ассемблера:"), m_syntaxCombo);

    m_insnLimit = new QSpinBox(backendGroup);
    m_insnLimit->setRange(50, 200000);
    m_insnLimit->setSingleStep(250);
    m_insnLimit->setToolTip(tr("Максимальное число инструкций на секцию (для плавной работы интерфейса)"));
    backendForm->addRow(tr("Лимит инструкций/секцию:"), m_insnLimit);

    disasmLayout->addWidget(backendGroup);

    // Tools Paths group
    auto *pathsGroup = new QGroupBox(tr("Пути к бинарным утилитам"), disasmContent);
    auto *pathsForm = new QFormLayout(pathsGroup);
    pathsForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pathsForm->setSpacing(10);

    // objdump path row
    {
        auto *row = new QWidget(pathsGroup);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        m_objdumpPath = new QLineEdit(row);
        m_objdumpPath->setPlaceholderText(tr("Оставьте пустым для автопоиска в PATH"));
        m_objdumpStatus = new QLabel(row);
        m_objdumpStatus->setObjectName("settingsStatusLabel");
        m_objdumpStatus->setMinimumWidth(120);
        m_objdumpStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto *browse = new QPushButton(tr("Обзор…"), row);
        browse->setFixedWidth(90);
        rowLayout->addWidget(m_objdumpPath, 1);
        rowLayout->addWidget(m_objdumpStatus);
        rowLayout->addWidget(browse);
        pathsForm->addRow(tr("Путь к objdump:"), row);
        connect(browse, &QPushButton::clicked, this, &SettingsDialog::onBrowseObjdump);
        connect(m_objdumpPath, &QLineEdit::textChanged, this, &SettingsDialog::updateDependencyStatus);
    }

    // radare2 path row
    {
        auto *row = new QWidget(pathsGroup);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        m_radare2Path = new QLineEdit(row);
        m_radare2Path->setPlaceholderText(tr("Путь к исполняемому файлу radare2 (r2)"));
        m_radare2Status = new QLabel(row);
        m_radare2Status->setObjectName("settingsStatusLabel");
        m_radare2Status->setMinimumWidth(120);
        m_radare2Status->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto *browse = new QPushButton(tr("Обзор…"), row);
        browse->setFixedWidth(90);
        rowLayout->addWidget(m_radare2Path, 1);
        rowLayout->addWidget(m_radare2Status);
        rowLayout->addWidget(browse);
        pathsForm->addRow(tr("Путь к radare2:"), row);
        connect(browse, &QPushButton::clicked, this, &SettingsDialog::onBrowseRadare2);
        connect(m_radare2Path, &QLineEdit::textChanged, this, &SettingsDialog::updateDependencyStatus);
    }

    // 'file' tool dependency
    {
        auto *row = new QWidget(pathsGroup);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        m_fileStatus = new QLabel(row);
        m_fileStatus->setObjectName("settingsStatusLabel");
        m_fileStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
        rowLayout->addWidget(m_fileStatus, 1);
        pathsForm->addRow(tr("Зависимость file(1):"), row);
    }

    disasmLayout->addWidget(pathsGroup);

    // radare2 options group
    auto *r2Group = new QGroupBox(tr("Параметры radare2"), disasmContent);
    auto *r2Form = new QFormLayout(r2Group);
    r2Form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    r2Form->setSpacing(10);

    m_r2AnalysisCombo = new QComboBox(r2Group);
    m_r2AnalysisCombo->addItem(tr("Без анализа (быстро)"), static_cast<int>(AppSettings::Radare2AnalysisLevel::None));
    m_r2AnalysisCombo->addItem(tr("aa (базовый анализ функций)"), static_cast<int>(AppSettings::Radare2AnalysisLevel::Aa));
    m_r2AnalysisCombo->addItem(tr("aaa (полный глубокий анализ)"), static_cast<int>(AppSettings::Radare2AnalysisLevel::Aaa));
    r2Form->addRow(tr("Уровень анализа:"), m_r2AnalysisCombo);

    m_r2PreCommands = new QPlainTextEdit(r2Group);
    m_r2PreCommands->setPlaceholderText(tr("Команды r2 перед разбором (по одной на строку):\ne asm.syntax=intel\ne asm.bits=64"));
    m_r2PreCommands->setFixedHeight(75);
    r2Form->addRow(tr("Команды r2:"), m_r2PreCommands);

    disasmLayout->addWidget(r2Group);
    disasmLayout->addStretch(1);

    disasmScroll->setWidget(disasmContent);
    auto *disasmTabRoot = new QVBoxLayout(disasmTab);
    disasmTabRoot->setContentsMargins(0, 0, 0, 0);
    disasmTabRoot->addWidget(disasmScroll);
    tabWidget->addTab(disasmTab, tr("Дизассемблер"));

    // ==========================================
    // TAB 2: Исключения файлов (File Filtering)
    // ==========================================
    auto *filterTab = new QWidget();
    auto *filterLayout = new QVBoxLayout(filterTab);
    filterLayout->setContentsMargins(20, 20, 20, 20);
    filterLayout->setSpacing(12);

    auto *filterTitle = new QLabel(tr("Фильтрация файлов и каталогов"), filterTab);
    filterTitle->setObjectName("settingsSectionTitle");
    filterTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #f3eefc;");
    filterLayout->addWidget(filterTitle);

    auto *filterHint = new QLabel(
        tr("Укажите шаблоны файлов и каталогов, которые не должны отображаться в дереве проекта.\n"
           "Каждый шаблон указывается с новой строки.\n"
           "Примеры: node_modules, .git, *.log, dist/, build/, __pycache__, *.tmp"),
        filterTab);
    filterHint->setObjectName("settingsHintLabel");
    filterHint->setWordWrap(true);
    filterHint->setStyleSheet("color: #a99dbd; line-height: 1.4;");
    filterLayout->addWidget(filterHint);

    m_excludedPatterns = new QPlainTextEdit(filterTab);
    m_excludedPatterns->setPlaceholderText(tr("node_modules\n.git\n*.log\nbuild/\ntarget/"));
    filterLayout->addWidget(m_excludedPatterns, 1);

    tabWidget->addTab(filterTab, tr("Фильтрация файлов"));

    // ==========================================
    // TAB 3: Интерфейс и О программе (Interface & About)
    // ==========================================
    auto *uiTab = new QWidget();
    auto *uiLayout = new QVBoxLayout(uiTab);
    uiLayout->setContentsMargins(20, 20, 20, 20);
    uiLayout->setSpacing(16);

    auto *langGroup = new QGroupBox(tr("Язык интерфейса"), uiTab);
    auto *langForm = new QFormLayout(langGroup);
    langForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_languageCombo = new QComboBox(langGroup);
    m_languageCombo->setPlaceholderText(tr("Выберите язык:"));
    for (auto const & locale : LanguageManager::supportedLanguages())
        m_languageCombo->addItem(QLocale(locale).nativeLanguageName(), QVariant::fromValue(locale));
    m_languageCombo->setMinimumWidth(250);
    langForm->addRow(tr("Язык приложения:"), m_languageCombo);
    uiLayout->addWidget(langGroup);

    auto *aboutBox = new QGroupBox(tr("О программе Amethyst"), uiTab);
    auto *aboutBoxLayout = new QVBoxLayout(aboutBox);
    aboutBoxLayout->setSpacing(10);

    auto *aboutText = new QLabel(
        tr("<p><b>Amethyst IDE</b> — среда низкоуровневой разработки с редактором кода, "
           "HEX-редактором и дизассемблером.</p>"
           "<p><b>Разработчик форка:</b> Atimenka<br>"
           "<b>Репозиторий GitHub:</b> <a href='https://github.com/Atimenka/Amethyst' style='color:#c084fc;'>https://github.com/Atimenka/Amethyst</a><br>"
           "<b>Оригинальный проект:</b> Cremniy (<a href='https://github.com/munirov/cremniy' style='color:#a99dbd;'>github.com/munirov/cremniy</a>)</p>"),
        aboutBox);
    aboutText->setWordWrap(true);
    aboutText->setOpenExternalLinks(true);
    aboutBoxLayout->addWidget(aboutText);

    auto *aboutLinkBtn = new QPushButton(tr("Открыть репозиторий GitHub (Atimenka/Amethyst)"), aboutBox);
    aboutLinkBtn->setCursor(Qt::PointingHandCursor);
    connect(aboutLinkBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka/Amethyst"));
    });
    aboutBoxLayout->addWidget(aboutLinkBtn);

    uiLayout->addWidget(aboutBox);
    uiLayout->addStretch(1);

    tabWidget->addTab(uiTab, tr("Интерфейс"));

    root->addWidget(tabWidget, 1);

    // ==========================================
    // Bottom Action Bar
    // ==========================================
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    m_testBtn = new QPushButton(tr("Тест инструментов"), this);
    m_testBtn->setCursor(Qt::PointingHandCursor);
    m_testBtn->setToolTip(tr("Проверить работоспособность objdump и radare2"));

    m_importBtn = new QPushButton(tr("Импорт…"), this);
    m_importBtn->setCursor(Qt::PointingHandCursor);

    m_exportBtn = new QPushButton(tr("Экспорт…"), this);
    m_exportBtn->setCursor(Qt::PointingHandCursor);

    btnRow->addWidget(m_testBtn);
    btnRow->addWidget(m_importBtn);
    btnRow->addWidget(m_exportBtn);
    btnRow->addStretch(1);

    m_cancelBtn = new QPushButton(tr("Отмена"), this);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);

    m_okBtn = new QPushButton(tr("Применить"), this);
    m_okBtn->setObjectName("SettingsApplyBtn");
    m_okBtn->setCursor(Qt::PointingHandCursor);
    m_okBtn->setDefault(true);

    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);

    connect(m_testBtn,   &QPushButton::clicked, this, &SettingsDialog::onTestTools);
    connect(m_exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportIni);
    connect(m_importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportIni);
    connect(m_okBtn,     &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_backendCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onBackendChanged);
    connect(m_insnLimit, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::updateDependencyStatus);
    connect(m_syntaxCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::updateDependencyStatus);
    connect(m_r2AnalysisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::updateDependencyStatus);
    connect(m_r2PreCommands, &QPlainTextEdit::textChanged, this, &SettingsDialog::updateDependencyStatus);
    connect(m_languageCombo, &QComboBox::currentTextChanged, this, [this] {
        onLanguageSwitched(m_languageCombo->currentData().toString());
    });

    loadFromSettings();
    updateUiEnabledState();
    updateDependencyStatus();
}

void SettingsDialog::onExportIni()
{
    const QString file = QFileDialog::getSaveFileName(
        this,
        tr("Экспорт настроек"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/amethyst-settings.ini",
        tr("Файлы INI (*.ini)"));
    if (file.isEmpty()) return;

    QString err;
    if (!AppSettings::exportToIni(file, &err)) {
        QMessageBox::warning(this, tr("Ошибка экспорта"), err.isEmpty() ? tr("Не удалось экспортировать настройки") : err);
        return;
    }
    QMessageBox::information(this, tr("Экспорт"), tr("Настройки успешно экспортированы в:\n%1").arg(file));
}

void SettingsDialog::onImportIni()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        tr("Импорт настроек"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("Файлы INI (*.ini)"));
    if (file.isEmpty()) return;

    QString err;
    if (!AppSettings::importFromIni(file, &err)) {
        QMessageBox::warning(this, tr("Ошибка импорта"), err.isEmpty() ? tr("Не удалось импортировать настройки") : err);
        return;
    }

    loadFromSettings();
    updateUiEnabledState();
    updateDependencyStatus();
    QMessageBox::information(this, tr("Импорт"), tr("Настройки успешно импортированы из:\n%1").arg(file));
}

void SettingsDialog::loadFromSettings()
{
    const auto backend = AppSettings::disasmBackend();
    const int want = static_cast<int>(backend);
    int idx = m_backendCombo->findData(want);
    if (idx < 0) idx = 0;
    m_backendCombo->setCurrentIndex(idx);

    m_objdumpPath->setText(AppSettings::objdumpPath());
    m_radare2Path->setText(AppSettings::radare2Path());

    m_insnLimit->setValue(AppSettings::disasmInsnLimitPerSection());

    {
        const int want = static_cast<int>(AppSettings::asmSyntax());
        const int idx = m_syntaxCombo->findData(want);
        m_syntaxCombo->setCurrentIndex(idx < 0 ? 0 : idx);
    }

    {
        const int want = static_cast<int>(AppSettings::radare2AnalysisLevel());
        const int idx = m_r2AnalysisCombo->findData(want);
        m_r2AnalysisCombo->setCurrentIndex(idx < 0 ? 0 : idx);
    }

    m_r2PreCommands->setPlainText(AppSettings::radare2PreCommands().replace(';', '\n'));

    m_excludedPatterns->setPlainText(AppSettings::excludedPatterns().join('\n'));

    const QString locale = AppSettings::getSettingsJson().value("language").toString();
    const int languageIndex = m_languageCombo->findData(locale);
    const QSignalBlocker blocker(m_languageCombo);
    m_languageCombo->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
}

void SettingsDialog::updateUiEnabledState()
{
    const bool useRadare2 =
        (m_backendCombo->currentData().toInt() == static_cast<int>(AppSettings::DisasmBackend::Radare2));

    m_radare2Path->setEnabled(true);
    m_objdumpPath->setEnabled(true);

    m_radare2Path->setToolTip(useRadare2 ? tr("Активный бэкенд") : tr("Неактивный бэкенд"));
    m_objdumpPath->setToolTip(useRadare2 ? tr("Неактивный бэкенд") : tr("Активный бэкенд"));

    m_r2AnalysisCombo->setEnabled(useRadare2);
    m_r2PreCommands->setEnabled(useRadare2);
}

void SettingsDialog::onBrowseObjdump()
{
    const QString cur = m_objdumpPath->text().trimmed();
    const QString file = QFileDialog::getOpenFileName(this, tr("Выбрать исполняемый файл objdump"), cur);
    if (!file.isEmpty())
        m_objdumpPath->setText(file);
}

void SettingsDialog::onBrowseRadare2()
{
    const QString cur = m_radare2Path->text().trimmed();
    const QString file = QFileDialog::getOpenFileName(this, tr("Выбрать исполняемый файл radare2 (r2)"), cur);
    if (!file.isEmpty())
        m_radare2Path->setText(file);
}

static bool runVersionCheck(const QString &exe, const QStringList &args, QString *out, QString *err)
{
    QProcess p;
    p.start(exe, args);
    if (!p.waitForStarted(2000))
        return false;
    if (!p.waitForFinished(4000))
        return false;
    if (out) *out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (err) *err = QString::fromUtf8(p.readAllStandardError()).trimmed();
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

void SettingsDialog::onTestTools()
{
    const QString objdumpExe = resolvedExecutable(m_objdumpPath->text(), "objdump");
    const QString r2Exe      = resolvedExecutable(m_radare2Path->text(), "r2");

    QStringList lines;

    // objdump
    {
        QString out, err;
        const bool ok = !objdumpExe.isEmpty() && runVersionCheck(objdumpExe, {"--version"}, &out, &err);
        lines << (ok ? tr("objdump: OK (%1)").arg(objdumpExe)
                     : tr("objdump: НЕ НАЙДЕН (%1)").arg(objdumpExe.isEmpty() ? tr("не найден в системе") : objdumpExe));
        if (!ok && !err.isEmpty())
            lines << "  " + err;
    }

    // r2
    {
        QString out, err;
        const bool ok = !r2Exe.isEmpty() && runVersionCheck(r2Exe, {"-v"}, &out, &err);
        lines << (ok ? tr("radare2: OK (%1)").arg(r2Exe)
                     : tr("radare2: НЕ НАЙДЕН (%1)").arg(r2Exe.isEmpty() ? tr("не найден в системе") : r2Exe));
        if (ok && !out.isEmpty())
            lines << "  " + out.split('\n').value(0);
        if (!ok && !err.isEmpty())
            lines << "  " + err;
    }

    QMessageBox::information(this, tr("Проверка инструментов"), lines.join('\n'));
    updateDependencyStatus();
}

void SettingsDialog::onAccept()
{
    const int backendInt = m_backendCombo->currentData().toInt();
    const auto backend = (backendInt == static_cast<int>(AppSettings::DisasmBackend::Radare2))
        ? AppSettings::DisasmBackend::Radare2
        : AppSettings::DisasmBackend::Objdump;

    AppSettings::setDisasmBackend(backend);
    AppSettings::setObjdumpPath(m_objdumpPath->text());
    AppSettings::setRadare2Path(m_radare2Path->text());

    AppSettings::setDisasmInsnLimitPerSection(m_insnLimit->value());
    AppSettings::setAsmSyntax(static_cast<AppSettings::AsmSyntax>(m_syntaxCombo->currentData().toInt()));
    AppSettings::setRadare2AnalysisLevel(static_cast<AppSettings::Radare2AnalysisLevel>(m_r2AnalysisCombo->currentData().toInt()));

    const QString pre = m_r2PreCommands->toPlainText()
                            .split('\n', Qt::SkipEmptyParts)
                            .join(';');
    AppSettings::setRadare2PreCommands(pre);

    // Excluded patterns
    {
        const QStringList patterns = m_excludedPatterns->toPlainText()
                                         .split('\n', Qt::SkipEmptyParts);
        AppSettings::setExcludedPatterns(patterns);
    }

    accept();
}

void SettingsDialog::onBackendChanged(int)
{
    updateUiEnabledState();
    updateDependencyStatus();
}

void SettingsDialog::updateDependencyStatus()
{
    // objdump
    {
        const QString resolved = resolvedExecutable(m_objdumpPath->text(), "objdump");
        const bool ok = isRunnableExecutable(resolved);
        setStatusLabel(m_objdumpStatus, ok, ok ? tr("найден") : tr("не найден"));
        m_objdumpStatus->setToolTip(ok ? resolved : tr("Не найден в PATH и не указан путь"));
    }

    // radare2
    {
        const QString resolved = resolvedExecutable(m_radare2Path->text(), "r2");
        const bool ok = isRunnableExecutable(resolved);
        setStatusLabel(m_radare2Status, ok, ok ? tr("найден") : tr("не найден"));
        m_radare2Status->setToolTip(ok ? resolved : tr("Не найден в PATH и не указан путь"));
    }

    // file(1) dependency
    {
        const QString fileExe = QStandardPaths::findExecutable("file");
        const bool ok = isRunnableExecutable(fileExe);
        setStatusLabel(m_fileStatus, ok, ok ? tr("найден") : tr("не найден"));
        m_fileStatus->setToolTip(ok ? fileExe : tr("Используется бэкендом objdump для автоопределения архитектуры"));
    }
}

void SettingsDialog::onLanguageSwitched(const QString &locale) {
    LanguageManager::instance().setLocale(locale);
    QMessageBox::information(this, tr("Информация"), tr("Пожалуйста, перезапустите IDE для применения нового языка."), QMessageBox::Ok);
}
