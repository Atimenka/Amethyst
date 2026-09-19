/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE / Cremniy IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#ifndef HELPMENU_H
#define HELPMENU_H

#include "ui/MenuBar/basemenu.h"

class HelpMenu : public BaseMenu
{
    Q_OBJECT
private:
    QAction* m_aboutAction{nullptr};
    QAction* m_githubAction{nullptr};
    QAction* m_issuesAction{nullptr};
public:
    HelpMenu();
    void setupConnections(IDEWindow* ideWind) override;
};

#endif // HELPMENU_H
