/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "diskimageviewerdialog.h"
#include "core/modules/ModuleManager.h"

#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEasingCurve>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QPropertyAnimation>
#include <QSplitter>
#include <QVBoxLayout>

static QString displayName() {
    return QCoreApplication::translate("DiskImageViewerDialog", "Просмотрщик образов дисков (.img / .iso)");
}

static bool registered = []() {
    ModuleManager::instance().registerModule<WindowBase>(
        &displayName, "", []() { return new DiskImageViewerDialog(); });
    return true;
}();

DiskImageViewerDialog::DiskImageViewerDialog(QWidget* parent)
    : WindowBase(parent)
{
    setWindowTitle(tr("Amethyst — Просмотрщик образов дисков и файловых систем"));
    resize(940, 620);
    setMinimumSize(780, 500);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    // Top File Selector Row
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    auto* pathLbl = new QLabel(tr("Файл образа:"), this);
    pathLbl->setStyleSheet("font-weight: bold; color: #c4b5fd; font-size: 12px;");
    topRow->addWidget(pathLbl);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(tr("Выберите образ (.img, .iso, .bin, .raw, .vmdk, .qcow2)..."));
    m_pathEdit->setReadOnly(true);
    topRow->addWidget(m_pathEdit, 1);

    m_browseBtn = new QPushButton(tr("Обзор…"), this);
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    m_browseBtn->setStyleSheet(
        "QPushButton { background: #7c3aed; color: #ffffff; border: none; "
        "border-radius: 5px; padding: 6px 14px; font-weight: bold; }"
        "QPushButton:hover { background: #9055ff; }");
    connect(m_browseBtn, &QPushButton::clicked, this, &DiskImageViewerDialog::onSelectFileClicked);
    topRow->addWidget(m_browseBtn);

    root->addLayout(topRow);

    // Status Label
    m_statusLabel = new QLabel(tr("Выберите образ диска для анализа разделов (поддерживаются ext1-4, ntfs, erofs, btrfs, fat, iso9660, squashfs)"), this);
    m_statusLabel->setStyleSheet("color: #a797be; font-size: 11px;");
    root->addWidget(m_statusLabel);

    // Main Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // Left Pane: Partitions Table & Details
    auto* leftPane = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* partHeader = new QLabel(tr("Разделы диска:"), leftPane);
    partHeader->setStyleSheet("font-weight: bold; color: #c4b5fd; font-size: 12px;");
    leftLayout->addWidget(partHeader);

    m_partitionsTable = new QTableWidget(0, 3, leftPane);
    m_partitionsTable->setHorizontalHeaderLabels({tr("№"), tr("ФС / Тип"), tr("Размер")});
    m_partitionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_partitionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partitionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_partitionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partitionsTable->verticalHeader()->setVisible(false);
    connect(m_partitionsTable, &QTableWidget::cellClicked, this, [this](int row, int) {
        onPartitionSelected(row);
    });
    leftLayout->addWidget(m_partitionsTable, 1);

    auto* detailsBox = new QGroupBox(tr("Свойства файловой системы"), leftPane);
    auto* detailsLayout = new QVBoxLayout(detailsBox);
    m_detailsLabel = new QLabel(tr("Нет активного раздела"), detailsBox);
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_detailsLabel->setStyleSheet("color: #d1c5e4; font-size: 11px;");
    detailsLayout->addWidget(m_detailsLabel);
    leftLayout->addWidget(detailsBox);

    splitter->addWidget(leftPane);

    // Right Pane: Files Browser
    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* filesTopRow = new QHBoxLayout();
    m_currentPathLabel = new QLabel(tr("Файловая структура раздела /"), rightPane);
    m_currentPathLabel->setStyleSheet("font-weight: bold; color: #f3eefc; font-size: 12px;");
    filesTopRow->addWidget(m_currentPathLabel);
    filesTopRow->addStretch(1);

    m_filterEdit = new QLineEdit(rightPane);
    m_filterEdit->setPlaceholderText(tr("Быстрый поиск файлов..."));
    m_filterEdit->setMaximumWidth(220);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &DiskImageViewerDialog::onFilterChanged);
    filesTopRow->addWidget(m_filterEdit);

    rightLayout->addLayout(filesTopRow);

    m_filesTable = new QTableWidget(0, 4, rightPane);
    m_filesTable->setHorizontalHeaderLabels({tr("Имя"), tr("Размер"), tr("Тип"), tr("Права / Inode")});
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_filesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_filesTable->verticalHeader()->setVisible(false);
    connect(m_filesTable, &QTableWidget::itemDoubleClicked, this, &DiskImageViewerDialog::onFileItemDoubleClicked);
    rightLayout->addWidget(m_filesTable, 1);

    // Bottom Action Row
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_extractBtn = new QPushButton(tr("📥 Извлечь выбранный файл..."), rightPane);
    m_extractBtn->setCursor(Qt::PointingHandCursor);
    m_extractBtn->setStyleSheet(
        "QPushButton { background: #7c3aed; color: #ffffff; border: none; "
        "border-radius: 5px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #9055ff; }");
    connect(m_extractBtn, &QPushButton::clicked, this, &DiskImageViewerDialog::onExtractClicked);
    btnRow->addWidget(m_extractBtn);

    btnRow->addStretch(1);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    root->addWidget(splitter, 1);
    setAcceptDrops(true);
}

void DiskImageViewerDialog::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DiskImageViewerDialog::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString localFile = urls.first().toLocalFile();
        if (!localFile.isEmpty()) {
            loadFile(localFile);
        }
    }
}

void DiskImageViewerDialog::showEvent(QShowEvent* event)
{
    WindowBase::showEvent(event);
    auto* anim = new QPropertyAnimation(this, "windowOpacity", this);
    anim->setDuration(220);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void DiskImageViewerDialog::onSelectFileClicked()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        tr("Открыть образ диска"),
        QString(),
        tr("Образы дисков (*.img *.iso *.bin *.raw *.vmdk *.qcow2);;Все файлы (*.*)")
    );
    if (!file.isEmpty()) {
        loadFile(file);
    }
}

void DiskImageViewerDialog::loadFile(const QString& path)
{
    m_filePath = path;
    m_pathEdit->setText(path);

    disk::DiskImageParser::parseImageFile(path, m_partitions);
    populatePartitions();
}

void DiskImageViewerDialog::populatePartitions()
{
    m_partitionsTable->setRowCount(0);

    if (m_partitions.isEmpty()) {
        m_statusLabel->setText(tr("Не удалось обнаружить разделы или образ пуст."));
        m_filesTable->setRowCount(0);
        m_detailsLabel->setText(tr("Разделы не найдены."));
        return;
    }

    m_statusLabel->setText(tr("Обнаружено разделов: %1 (Файл: %2)")
                               .arg(m_partitions.size())
                               .arg(QFileInfo(m_filePath).fileName()));

    for (int i = 0; i < m_partitions.size(); ++i) {
        const auto& part = m_partitions[i];
        m_partitionsTable->insertRow(i);

        auto* numItem = new QTableWidgetItem(QString::number(part.index));
        auto* fsItem  = new QTableWidgetItem(part.fsType);
        auto* sizeItem = new QTableWidgetItem(
            part.sizeBytes >= 1024 * 1024 * 1024
                ? QString("%1 ГБ").arg(part.sizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1)
                : QString("%1 МБ").arg(part.sizeBytes / (1024.0 * 1024.0), 0, 'f', 1)
        );

        m_partitionsTable->setItem(i, 0, numItem);
        m_partitionsTable->setItem(i, 1, fsItem);
        m_partitionsTable->setItem(i, 2, sizeItem);
    }

    m_partitionsTable->selectRow(0);
    onPartitionSelected(0);
}

void DiskImageViewerDialog::onPartitionSelected(int row)
{
    if (row < 0 || row >= m_partitions.size()) return;
    m_activePartitionIndex = row;
    m_partitionsTable->selectRow(row);

    const auto& part = m_partitions[row];
    updateDetails(part);
    populateFiles(part);
}

void DiskImageViewerDialog::updateDetails(const disk::PartitionInfo& part)
{
    QString info = QString("<b>Раздел:</b> %1<br>"
                           "<b>Файловая система:</b> <span style='color:#c084fc;'>%2</span><br>"
                           "<b>Метка:</b> %3<br>"
                           "<b>Размер:</b> %4 байт<br>"
                           "<b>Смещение:</b> 0x%5<br>")
                       .arg(part.name)
                       .arg(part.fsType)
                       .arg(part.label.isEmpty() ? tr("(нет)") : part.label)
                       .arg(part.sizeBytes)
                       .arg(part.startByte, 0, 16);

    for (auto it = part.details.constBegin(); it != part.details.constEnd(); ++it) {
        info += QString("<b>%1:</b> %2<br>").arg(it.key()).arg(it.value());
    }

    m_detailsLabel->setText(info);
}

void DiskImageViewerDialog::populateFiles(const disk::PartitionInfo& part)
{
    m_filesTable->setRowCount(0);
    const QString filter = m_filterEdit->text().trimmed().toLower();

    int row = 0;
    for (const auto& entry : part.rootEntries) {
        if (!filter.isEmpty() && !entry.name.toLower().contains(filter)) {
            continue;
        }

        m_filesTable->insertRow(row);

        QString iconText = entry.isDirectory ? "📁 " : "📄 ";
        auto* nameItem = new QTableWidgetItem(iconText + entry.name);
        nameItem->setData(Qt::UserRole, entry.fullPath);

        QString sizeStr = entry.isDirectory ? "-" : QString("%1 байт").arg(entry.size);
        if (entry.size >= 1024 * 1024) {
            sizeStr = QString("%1 МБ").arg(entry.size / (1024.0 * 1024.0), 0, 'f', 2);
        } else if (entry.size >= 1024) {
            sizeStr = QString("%1 КБ").arg(entry.size / 1024.0, 0, 'f', 1);
        }

        auto* sizeItem = new QTableWidgetItem(sizeStr);
        auto* typeItem = new QTableWidgetItem(entry.isDirectory ? tr("Папка") : (entry.isSymlink ? tr("Ссылка") : tr("Файл")));
        auto* permItem = new QTableWidgetItem(entry.permissions.isEmpty() ? QString("ino: %1").arg(entry.inode) : entry.permissions);

        m_filesTable->setItem(row, 0, nameItem);
        m_filesTable->setItem(row, 1, sizeItem);
        m_filesTable->setItem(row, 2, typeItem);
        m_filesTable->setItem(row, 3, permItem);
        ++row;
    }

    m_currentPathLabel->setText(tr("Содержимое раздела %1 / (%2 элементов)").arg(part.index).arg(row));
}

void DiskImageViewerDialog::onFilterChanged(const QString&)
{
    if (m_activePartitionIndex >= 0 && m_activePartitionIndex < m_partitions.size()) {
        populateFiles(m_partitions[m_activePartitionIndex]);
    }
}

void DiskImageViewerDialog::onFileItemDoubleClicked(QTableWidgetItem* item)
{
    if (!item) return;
    int row = item->row();
    auto* nameItem = m_filesTable->item(row, 0);
    if (!nameItem) return;

    QString fileName = nameItem->text();
    if (fileName.startsWith("📁 ")) {
        QMessageBox::information(this, tr("Папка"), tr("Каталог: %1").arg(fileName.mid(3)));
    } else {
        onExtractClicked();
    }
}

void DiskImageViewerDialog::onExtractClicked()
{
    int row = m_filesTable->currentRow();
    if (row < 0 || m_activePartitionIndex >= m_partitions.size()) {
        QMessageBox::information(this, tr("Извлечение"), tr("Пожалуйста, выберите файл для извлечения."));
        return;
    }

    auto* nameItem = m_filesTable->item(row, 0);
    if (!nameItem) return;

    QString fileName = nameItem->text();
    if (fileName.startsWith("📄 ") || fileName.startsWith("📁 ")) fileName = fileName.mid(3);

    const auto& part = m_partitions[m_activePartitionIndex];
    disk::FsEntry targetEntry;
    for (const auto& e : part.rootEntries) {
        if (e.name == fileName) {
            targetEntry = e;
            break;
        }
    }

    const QString destFile = QFileDialog::getSaveFileName(this, tr("Сохранить извлечённый файл"), fileName);
    if (destFile.isEmpty()) return;

    QByteArray extracted = disk::DiskImageParser::extractFileData(m_filePath, part, targetEntry);

    QFile outFile(destFile);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(extracted);
        outFile.close();
        QMessageBox::information(this, tr("Успех"), tr("Файл успешно сохранён:\n%1").arg(destFile));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось записать файл."));
    }
}
