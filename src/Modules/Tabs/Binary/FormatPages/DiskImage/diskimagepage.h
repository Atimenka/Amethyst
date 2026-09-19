/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#ifndef DISKIMAGEPAGE_H
#define DISKIMAGEPAGE_H

#include "formatpage.h"
#include "core/disk/disk_image_parser.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

class PartitionMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PartitionMapWidget(QWidget* parent = nullptr);

    void setPartitions(const QVector<disk::PartitionInfo>& partitions);
    void setSelectedPartition(int index);

signals:
    void partitionClicked(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QVector<disk::PartitionInfo> m_partitions;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
};

class DiskImagePage : public FormatPage
{
    Q_OBJECT

public:
    explicit DiskImagePage(QWidget *parent = nullptr);

    QString pageName() const override { return tr("Образ диска / FS"); }

    void setPageData(QByteArray& data) override;
    QByteArray getPageData() const override;
    void setSelection(qint64 pos, qint64 length) override { Q_UNUSED(pos); Q_UNUSED(length); }

    void loadExternalImage(const QString& filePath);

private slots:
    void onPartitionSelected(int index);
    void onFileItemDoubleClicked(QTableWidgetItem* item);
    void onExtractClicked();
    void onOpenInHexClicked();
    void onFilterChanged(const QString& text);
    void onOpenAnotherImageClicked();

private:
    void populatePartitions();
    void populateFiles(const disk::PartitionInfo& part);
    void updateDetails(const disk::PartitionInfo& part);

    QByteArray m_currentData;
    QString m_loadedFilePath;
    QVector<disk::PartitionInfo> m_partitions;
    int m_activePartitionIndex = 0;

    // UI elements
    PartitionMapWidget* m_partitionMap = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTableWidget* m_partitionsTable = nullptr;
    QLabel* m_fsDetailsLabel = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QTableWidget* m_filesTable = nullptr;
    QLabel* m_currentPathLabel = nullptr;
    QPushButton* m_extractBtn = nullptr;
    QPushButton* m_openHexBtn = nullptr;
    QPushButton* m_openImageBtn = nullptr;
};

#endif // DISKIMAGEPAGE_H
