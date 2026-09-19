/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 *
 * Modified by Ilya (https://github.com/kykyrudza) on 2026-05-12
 */

#ifndef WELCOME_FORM_H
#define WELCOME_FORM_H

#include <QStackedWidget>
#include <QWidget>

class RecentProjectsPage;
class CreateProjectPage;

class WelcomeForm : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeForm(QWidget* parent = nullptr);
    ~WelcomeForm() override = default;

    void openProject(const QString& path, const QString& language = {});

private:
    QStackedWidget*     m_stack;
    RecentProjectsPage* m_recentPage;
    CreateProjectPage*  m_createPage;

    void switchPage(int targetIndex);
    void loadStyles();
};

#endif /* WELCOME_FORM_H */
