/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "aboutdialog.h"

#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("О программе Amethyst"));
    setFixedSize(540, 420);
    setWindowIcon(QIcon(":/icons/icon.svg"));
    setObjectName("AboutDialog");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(14);

    // Header: icon + name & developer
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    auto *iconLabel = new QLabel(this);
    QPixmap iconPixmap = QIcon(":/icons/icon.svg").pixmap(64, 64);
    if (iconPixmap.isNull()) {
        iconPixmap = QIcon(":/icons/amethyst.ico").pixmap(64, 64);
    }
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(64, 64);
    headerLayout->addWidget(iconLabel);

    auto *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(3);

    auto *titleLabel = new QLabel("Amethyst IDE", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #ffffff;");

    auto *devBadge = new QLabel(tr("Разработчик: <b>Atimenka</b>"), this);
    devBadge->setStyleSheet("color: #c084fc; font-size: 13px; font-weight: 500;");

    auto *versionLabel = new QLabel(tr("Версия 0.3.1 (Windows Edition) • Форк Cremniy"), this);
    versionLabel->setStyleSheet("color: #9d8eb5; font-size: 12px;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(devBadge);
    titleLayout->addWidget(versionLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // Separator
    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #38294f; background-color: #38294f; max-height: 1px;");
    mainLayout->addWidget(sep);

    // Description text
    auto *descLabel = new QLabel(
        tr("<p style='line-height: 1.4;'><b>Amethyst</b> — современная среда для низкоуровневой разработки, "
           "системного программирования и реверс-инжиниринга. "
           "Объединяет в едином приложении мощный редактор кода, встроенный HEX-редактор "
           "и дизассемблер с поддержкой форматов PE, ELF, Mach-O.</p>"
           "<p style='line-height: 1.4;'>Проект является развитием и форком <b>Cremniy</b> "
           "(оригинальный автор — Munirov), оптимизированным для нативной работы под Windows и Linux.</p>"
           "<table cellpadding='2' style='color: #cccccc;'>"
           "<tr><td><b>Разработчик форка:</b></td><td><a href='https://github.com/Atimenka' style='color: #c084fc; text-decoration: none;'>Atimenka (GitHub)</a></td></tr>"
           "<tr><td><b>Репозиторий Amethyst:</b></td><td><a href='https://github.com/Atimenka/Amethyst' style='color: #c084fc; text-decoration: none;'>https://github.com/Atimenka/Amethyst</a></td></tr>"
           "<tr><td><b>Оригинальный проект:</b></td><td><a href='https://github.com/munirov/cremniy' style='color: #9d8eb5; text-decoration: none;'>https://github.com/munirov/cremniy</a></td></tr>"
           "<tr><td><b>Лицензия:</b></td><td>GNU General Public License v3.0</td></tr>"
           "</table>"),
        this);
    descLabel->setWordWrap(true);
    descLabel->setOpenExternalLinks(true);
    mainLayout->addWidget(descLabel);

    mainLayout->addStretch();

    // Buttons bar
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    auto *githubBtn = new QPushButton(tr("Репозиторий GitHub"), this);
    githubBtn->setCursor(Qt::PointingHandCursor);
    githubBtn->setStyleSheet(
        "QPushButton { background: #261a38; color: #c084fc; border: 1px solid #6b21a8; "
        "border-radius: 6px; padding: 6px 14px; font-weight: 500; }"
        "QPushButton:hover { background: #3b2060; border-color: #a855f7; color: #ffffff; }");
    connect(githubBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka/Amethyst"));
    });
    btnLayout->addWidget(githubBtn);

    auto *devProfileBtn = new QPushButton(tr("Профиль Atimenka"), this);
    devProfileBtn->setCursor(Qt::PointingHandCursor);
    devProfileBtn->setStyleSheet(
        "QPushButton { background: #261a38; color: #c084fc; border: 1px solid #6b21a8; "
        "border-radius: 6px; padding: 6px 14px; font-weight: 500; }"
        "QPushButton:hover { background: #3b2060; border-color: #a855f7; color: #ffffff; }");
    connect(devProfileBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka"));
    });
    btnLayout->addWidget(devProfileBtn);

    btnLayout->addStretch();

    auto *closeBtn = new QPushButton(tr("Закрыть"), this);
    closeBtn->setDefault(true);
    closeBtn->setMinimumWidth(90);
    closeBtn->setStyleSheet(
        "QPushButton { background: #7c3aed; color: #ffffff; border: none; "
        "border-radius: 6px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #9055ff; }"
        "QPushButton:pressed { background: #6222cc; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void AboutDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    auto *anim = new QPropertyAnimation(this, "windowOpacity", this);
    anim->setDuration(240);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AboutDialog::showAbout(QWidget *parent) {
    AboutDialog dlg(parent);
    dlg.exec();
}
