/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE / Cremniy IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#ifndef RECENT_PROJECTS_PAGE_H
#define RECENT_PROJECTS_PAGE_H

#include <QVBoxLayout>
#include <QWidget>

class QScrollArea;

class RecentProjectsPage : public QWidget {
    Q_OBJECT

public:
    explicit RecentProjectsPage(QWidget* parent = nullptr);

    void reload();

signals:
    void openProjectRequested(const QString& path);
    void newProjectRequested();

private:
    QVBoxLayout* m_cardsLayout{nullptr};
    QWidget*     m_emptyStateWidget{nullptr};
    QScrollArea* m_scrollArea{nullptr};

    void clearCards() const;
};

#endif /* RECENT_PROJECTS_PAGE_H */
