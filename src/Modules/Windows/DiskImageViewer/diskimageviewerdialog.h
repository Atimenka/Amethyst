/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#ifndef DISKIMAGEVIEWERDIALOG_H
#define DISKIMAGEVIEWERDIALOG_H

#include "core/modules/WindowBase.h"
#include "core/disk/disk_image_parser.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVector>

class QDragEnterEvent;
class QDropEvent;

class DiskImageViewerDialog final : public WindowBase
{
    Q_OBJECT

public:
    explicit DiskImageViewerDialog(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onSelectFileClicked();
    void onPartitionSelected(int row);
    void onFileItemDoubleClicked(QTableWidgetItem* item);
    void onExtractClicked();
    void onFilterChanged(const QString& filter);

private:
    void loadFile(const QString& path);
    void populatePartitions();
    void populateFiles(const disk::PartitionInfo& part);
    void updateDetails(const disk::PartitionInfo& part);

    QString m_filePath;
    QVector<disk::PartitionInfo> m_partitions;
    int m_activePartitionIndex = 0;

    // UI
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTableWidget* m_partitionsTable = nullptr;
    QLabel* m_detailsLabel = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QTableWidget* m_filesTable = nullptr;
    QLabel* m_currentPathLabel = nullptr;
    QPushButton* m_extractBtn = nullptr;
};

#endif // DISKIMAGEVIEWERDIALOG_H
