/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE / Cremniy IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "project_card.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

ProjectCard::ProjectCard(const utils::RecentProject& project, QWidget* parent)
    : QWidget(parent)
    , m_path(project.path)
{
    setObjectName("ProjectCard");
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::PointingHandCursor);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(14, 10, 14, 10);
    root->setSpacing(14);

    auto* badge = new QLabel(shortLang(project.language), this);
    badge->setObjectName("ProjectCardBadge");
    badge->setFixedSize(38, 38);
    badge->setAlignment(Qt::AlignCenter);

    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(3);
    infoLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);
    nameRow->setContentsMargins(0, 0, 0, 0);

    auto* nameLabel = new QLabel(project.name, this);
    nameLabel->setObjectName("ProjectCardName");
    nameRow->addWidget(nameLabel);

    if (!project.language.isEmpty()) {
        auto* langLabel = new QLabel(project.language, this);
        langLabel->setObjectName("ProjectCardLang");
        nameRow->addWidget(langLabel);
    }
    nameRow->addStretch();

    auto* pathLabel = new QLabel(project.path, this);
    pathLabel->setObjectName("ProjectCardPath");
    pathLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    infoLayout->addLayout(nameRow);
    infoLayout->addWidget(pathLabel);

    if (project.lastOpened.isValid()) {
        const auto diff = project.lastOpened.secsTo(QDateTime::currentDateTime());
        QString dateText;
        if (diff < 60) {
            dateText = tr("только что");
        } else if (diff < 3600) {
            dateText = tr("%1 мин. назад").arg(diff / 60);
        } else if (diff < 86400) {
            dateText = tr("%1 ч. назад").arg(diff / 3600);
        } else if (diff < 86400 * 7) {
            dateText = tr("%1 дн. назад").arg(diff / 86400);
        } else {
            dateText = project.lastOpened.toString("dd.MM.yyyy");
        }

        auto* dateLabel = new QLabel(dateText, this);
        dateLabel->setObjectName("ProjectCardDate");
        infoLayout->addWidget(dateLabel);
    }

    auto* openBtn = new QPushButton(tr("Открыть"), this);
    openBtn->setObjectName("ProjectCardOpenBtn");
    openBtn->setToolTip(tr("Открыть этот проект"));
    connect(openBtn, &QPushButton::clicked, this, [this] {
        emit openRequested(m_path);
    });

    auto* removeBtn = new QPushButton("✕", this);
    removeBtn->setObjectName("ProjectCardRemoveBtn");
    removeBtn->setToolTip(tr("Удалить из недавних проектов"));
    removeBtn->setFixedSize(26, 26);
    connect(removeBtn, &QPushButton::clicked, this, [this] {
        emit removeRequested(m_path);
    });

    root->addWidget(badge);
    root->addLayout(infoLayout, 1);
    root->addWidget(openBtn);
    root->addWidget(removeBtn);
}

void ProjectCard::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    emit openRequested(m_path);
}

QString ProjectCard::shortLang(const QString& lang)
{
    const QString l = lang.toLower();
    if (l == "c") return "C";
    if (l == "c++" || l == "cpp") return "C++";
    if (l == "assembly" || l == "asm") return "ASM";
    if (l == "rust") return "RS";
    if (l == "python") return "PY";
    if (l == "go") return "GO";
    if (l == "zig") return "ZIG";
    return lang.left(3).toUpper();
}
