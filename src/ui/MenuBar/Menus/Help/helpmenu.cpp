/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "helpmenu.h"
#include "ui/MenuBar/menufactory.h"
#include "dialogs/aboutdialog.h"
#include <QDesktopServices>
#include <QUrl>

static bool registered = [](){
    MenuFactory::instance().registerMenu("8", [](){
        return new HelpMenu();
    });
    return true;
}();

HelpMenu::HelpMenu() : BaseMenu(tr("Справка")) {
    m_aboutAction = new QAction(tr("О программе Amethyst..."), this);
    m_githubAction = new QAction(tr("Репозиторий проекта на GitHub..."), this);
    m_issuesAction = new QAction(tr("Сообщить об ошибке (GitHub Issues)..."), this);

    this->addAction(m_aboutAction);
    this->addSeparator();
    this->addAction(m_githubAction);
    this->addAction(m_issuesAction);
}

void HelpMenu::setupConnections(IDEWindow* ideWind) {
    connect(m_aboutAction, &QAction::triggered, ideWind, [ideWind]() {
        AboutDialog::showAbout(ideWind);
    });
    connect(m_githubAction, &QAction::triggered, ideWind, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka/Amethyst"));
    });
    connect(m_issuesAction, &QAction::triggered, ideWind, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Atimenka/Amethyst/issues"));
    });
}
