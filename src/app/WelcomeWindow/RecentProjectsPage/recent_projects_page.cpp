/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "recent_projects_page.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "dialogs/aboutdialog.h"
#include "project_card.h"
#include "projects_history_manager.h"

RecentProjectsPage::RecentProjectsPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("RecentProjectsPage");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    // ==========================================
    // 1. Modern Amethyst Header
    // ==========================================
    auto* header = new QWidget(this);
    header->setObjectName("WelcomeHeader");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 12, 16, 12);
    headerLayout->setSpacing(14);

    auto* logoLabel = new QLabel(header);
    logoLabel->setObjectName("WelcomeLogo");
    QPixmap logoPixmap(":/icons/icon.svg");
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(44, 44, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setFixedSize(44, 44);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    titleCol->setContentsMargins(0, 0, 0, 0);

    auto* appTitle = new QLabel("Amethyst", header);
    appTitle->setObjectName("WelcomeTitle");

    auto* appSub = new QLabel(tr("Среда низкоуровневой разработки и реверса • Windows Edition"), header);
    appSub->setObjectName("WelcomeSubtitle");

    titleCol->addWidget(appTitle);
    titleCol->addWidget(appSub);

    headerLayout->addWidget(logoLabel);
    headerLayout->addLayout(titleCol);
    headerLayout->addStretch(1);

    // Developer badge
    auto* devBadge = new QPushButton(tr("Разработчик: Atimenka"), header);
    devBadge->setObjectName("WelcomeDevBadge");
    devBadge->setCursor(Qt::PointingHandCursor);
    devBadge->setToolTip(tr("Открыть профиль разработчика: https://github.com/Atimenka"));
    connect(devBadge, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka"));
    });
    headerLayout->addWidget(devBadge);

    // GitHub repository button
    auto* githubBtn = new QPushButton(tr("GitHub"), header);
    githubBtn->setObjectName("WelcomeGithubBtn");
    githubBtn->setCursor(Qt::PointingHandCursor);
    githubBtn->setToolTip(tr("Открыть репозиторий проекта на GitHub"));
    connect(githubBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka/Amethyst"));
    });
    headerLayout->addWidget(githubBtn);

    // About dialog button
    auto* aboutBtn = new QPushButton(tr("О программе"), header);
    aboutBtn->setObjectName("WelcomeAboutBtn");
    aboutBtn->setCursor(Qt::PointingHandCursor);
    aboutBtn->setToolTip(tr("Информация о приложении Amethyst"));
    connect(aboutBtn, &QPushButton::clicked, this, [this]() {
        AboutDialog::showAbout(this);
    });
    headerLayout->addWidget(aboutBtn);

    root->addWidget(header);

    // ==========================================
    // 2. Center Content Area (Cards or Empty State)
    // ==========================================
    auto* sectionTitle = new QLabel(tr("Недавние проекты"), this);
    sectionTitle->setObjectName("RecentProjectsTitle");
    root->addWidget(sectionTitle);

    // Cards scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("CardsScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    auto* cardsContainer = new QWidget();
    cardsContainer->setObjectName("CardsContainer");
    m_cardsLayout = new QVBoxLayout(cardsContainer);
    m_cardsLayout->setContentsMargins(4, 4, 4, 4);
    m_cardsLayout->setSpacing(8);
    m_cardsLayout->addStretch();
    m_scrollArea->setWidget(cardsContainer);

    root->addWidget(m_scrollArea, 1);

    // Empty state widget
    m_emptyStateWidget = new QWidget(this);
    m_emptyStateWidget->setObjectName("WelcomeEmptyState");
    auto* emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    emptyLayout->setContentsMargins(20, 30, 20, 30);
    emptyLayout->setSpacing(12);
    emptyLayout->setAlignment(Qt::AlignCenter);

    auto* emptyLogo = new QLabel(m_emptyStateWidget);
    if (!logoPixmap.isNull()) {
        emptyLogo->setPixmap(logoPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    emptyLogo->setAlignment(Qt::AlignCenter);

    auto* emptyTitle = new QLabel(tr("Добро пожаловать в Amethyst"), m_emptyStateWidget);
    emptyTitle->setObjectName("EmptyStateTitle");
    emptyTitle->setAlignment(Qt::AlignCenter);

    auto* emptyText = new QLabel(
        tr("Среда для разработки низкоуровневого ПО, анализа байт-кода и дизассемблирования.\n"
           "Создайте новый проект или откройте существующую папку для начала работы."),
        m_emptyStateWidget);
    emptyText->setObjectName("EmptyStateText");
    emptyText->setAlignment(Qt::AlignCenter);

    auto* emptyDevText = new QLabel(
        tr("Разработчик: Atimenka • Amethyst IDE"),
        m_emptyStateWidget);
    emptyDevText->setObjectName("EmptyStateDevText");
    emptyDevText->setAlignment(Qt::AlignCenter);

    auto* emptyBtnRow = new QHBoxLayout();
    emptyBtnRow->setSpacing(12);
    emptyBtnRow->setAlignment(Qt::AlignCenter);

    auto* emptyNewBtn = new QPushButton(tr("＋ Создать новый проект"), m_emptyStateWidget);
    emptyNewBtn->setObjectName("EmptyStateNewBtn");
    emptyNewBtn->setCursor(Qt::PointingHandCursor);
    connect(emptyNewBtn, &QPushButton::clicked, this, &RecentProjectsPage::newProjectRequested);

    auto* emptyOpenBtn = new QPushButton(tr("📂 Открыть папку..."), m_emptyStateWidget);
    emptyOpenBtn->setObjectName("EmptyStateOpenBtn");
    emptyOpenBtn->setCursor(Qt::PointingHandCursor);
    connect(emptyOpenBtn, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Открыть папку проекта"), QString(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            emit openProjectRequested(dir);
        }
    });

    emptyBtnRow->addWidget(emptyNewBtn);
    emptyBtnRow->addWidget(emptyOpenBtn);

    emptyLayout->addWidget(emptyLogo);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addWidget(emptyDevText);
    emptyLayout->addSpacing(10);
    emptyLayout->addLayout(emptyBtnRow);

    root->addWidget(m_emptyStateWidget, 1);

    // ==========================================
    // 3. Bottom Toolbar
    // ==========================================
    auto* bottomBar = new QWidget(this);
    bottomBar->setObjectName("WelcomeBottomBar");
    auto* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 8, 0, 0);
    bottomLayout->setSpacing(12);

    auto* infoLabel = new QLabel(tr("Amethyst v0.3.1 (Windows Edition) • Atimenka"), bottomBar);
    infoLabel->setObjectName("WelcomeInfoLabel");
    bottomLayout->addWidget(infoLabel);
    bottomLayout->addStretch(1);

    auto* openBtn = new QPushButton(tr("Открыть папку..."), bottomBar);
    openBtn->setObjectName("WelcomeOpenBtn");
    openBtn->setCursor(Qt::PointingHandCursor);
    connect(openBtn, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Открыть папку проекта"), QString(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            emit openProjectRequested(dir);
        }
    });

    auto* newBtn = new QPushButton(tr("＋ Создать проект"), bottomBar);
    newBtn->setObjectName("WelcomeNewBtn");
    newBtn->setCursor(Qt::PointingHandCursor);
    connect(newBtn, &QPushButton::clicked, this, &RecentProjectsPage::newProjectRequested);

    bottomLayout->addWidget(openBtn);
    bottomLayout->addWidget(newBtn);

    root->addWidget(bottomBar);

    reload();
}

void RecentProjectsPage::reload()
{
    clearCards();

    const auto projects = utils::ProjectsHistoryManager::instance().recentProjects();

    if (projects.isEmpty()) {
        m_scrollArea->hide();
        m_emptyStateWidget->show();
        return;
    }

    m_emptyStateWidget->hide();
    m_scrollArea->show();

    for (const auto& project : projects) {
        if (!QFileInfo::exists(project.path)) {
            continue;
        }

        auto* card = new ProjectCard(project);
        connect(card, &ProjectCard::openRequested, this, &RecentProjectsPage::openProjectRequested);
        connect(card, &ProjectCard::removeRequested, this, [this](const QString& path) {
            utils::ProjectsHistoryManager::instance().remove(path);
            reload();
        });

        m_cardsLayout->insertWidget(m_cardsLayout->count() - 1, card);
    }
}

void RecentProjectsPage::clearCards() const
{
    while (m_cardsLayout->count() > 1) {
        QLayoutItem* item = m_cardsLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}
