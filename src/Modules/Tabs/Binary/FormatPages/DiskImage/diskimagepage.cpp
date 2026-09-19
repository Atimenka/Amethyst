/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "diskimagepage.h"
#include "formatpagefactory.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>

static bool registered = [](){
    FormatPageFactory::instance().registerPage("5", [](){
        return new DiskImagePage();
    });
    return true;
}();

// ==========================================
// Partition Visual Map Widget
// ==========================================
PartitionMapWidget::PartitionMapWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(44);
    setMouseTracking(true);
}

void PartitionMapWidget::setPartitions(const QVector<disk::PartitionInfo>& partitions)
{
    m_partitions = partitions;
    m_selectedIndex = partitions.isEmpty() ? -1 : 0;
    update();
}

void PartitionMapWidget::setSelectedPartition(int index)
{
    if (index >= 0 && index < m_partitions.size()) {
        m_selectedIndex = index;
        update();
    }
}

void PartitionMapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect().adjusted(1, 1, -1, -1);
    p.fillRect(r, QColor("#140f1d"));
    p.setPen(QColor("#382452"));
    p.drawRoundedRect(r, 6, 6);

    if (m_partitions.isEmpty()) {
        p.setPen(QColor("#8c7e9f"));
        p.drawText(r, Qt::AlignCenter, tr("Нет разделов или образ не загружен"));
        return;
    }

    quint64 totalBytes = 0;
    for (const auto& part : m_partitions) {
        totalBytes += (part.sizeBytes > 0) ? part.sizeBytes : 1024 * 1024;
    }
    if (totalBytes == 0) totalBytes = 1;

    int curX = r.left() + 2;
    int availW = r.width() - 4;

    static const QColor fsColors[] = {
        QColor("#7c3aed"), // ext4 (purple)
        QColor("#2563eb"), // ntfs (blue)
        QColor("#059669"), // fat32 (green)
        QColor("#d97706"), // erofs (amber)
        QColor("#9333ea"), // btrfs (violet)
        QColor("#0891b2"), // iso (cyan)
        QColor("#dc2626"), // squashfs (red)
        QColor("#4b5563")  // other
    };

    for (int i = 0; i < m_partitions.size(); ++i) {
        const auto& part = m_partitions[i];
        quint64 pBytes = (part.sizeBytes > 0) ? part.sizeBytes : 1024 * 1024;
        int w = qMax(40, static_cast<int>((double)pBytes / totalBytes * availW));
        if (i == m_partitions.size() - 1) {
            w = r.right() - curX - 2;
        }

        QRect partRect(curX, r.top() + 3, w, r.height() - 6);
        QColor baseColor = fsColors[i % 8];

        if (i == m_selectedIndex) {
            baseColor = baseColor.lighter(130);
        } else if (i == m_hoveredIndex) {
            baseColor = baseColor.lighter(115);
        }

        p.fillRect(partRect, baseColor);

        // Selection / hover border
        if (i == m_selectedIndex) {
            p.setPen(QPen(QColor("#ffffff"), 2));
            p.drawRect(partRect);
        } else {
            p.setPen(QColor("#1f142b"));
            p.drawRect(partRect);
        }

        // Label
        p.setPen(QColor("#ffffff"));
        QFont f = p.font();
        f.setPointSize(9);
        f.setBold(i == m_selectedIndex);
        p.setFont(f);

        QString label = QString("%1: %2").arg(part.index).arg(part.fsType);
        p.drawText(partRect.adjusted(4, 0, -4, 0), Qt::AlignCenter, p.fontMetrics().elidedText(label, Qt::ElideRight, partRect.width() - 8));

        curX += w;
    }
}

void PartitionMapWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_partitions.isEmpty()) return;

    quint64 totalBytes = 0;
    for (const auto& part : m_partitions) totalBytes += (part.sizeBytes > 0) ? part.sizeBytes : 1024 * 1024;
    if (totalBytes == 0) return;

    QRect r = rect().adjusted(2, 2, -2, -2);
    int curX = r.left();
    int availW = r.width();

    for (int i = 0; i < m_partitions.size(); ++i) {
        quint64 pBytes = (m_partitions[i].sizeBytes > 0) ? m_partitions[i].sizeBytes : 1024 * 1024;
        int w = qMax(40, static_cast<int>((double)pBytes / totalBytes * availW));
        if (i == m_partitions.size() - 1) w = r.right() - curX;

        QRect partRect(curX, r.top(), w, r.height());
        if (partRect.contains(event->pos())) {
            m_selectedIndex = i;
            update();
            emit partitionClicked(i);
            break;
        }
        curX += w;
    }
}

void PartitionMapWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_partitions.isEmpty()) return;

    quint64 totalBytes = 0;
    for (const auto& part : m_partitions) totalBytes += (part.sizeBytes > 0) ? part.sizeBytes : 1024 * 1024;
    if (totalBytes == 0) return;

    QRect r = rect().adjusted(2, 2, -2, -2);
    int curX = r.left();
    int availW = r.width();
    int newHover = -1;

    for (int i = 0; i < m_partitions.size(); ++i) {
        quint64 pBytes = (m_partitions[i].sizeBytes > 0) ? m_partitions[i].sizeBytes : 1024 * 1024;
        int w = qMax(40, static_cast<int>((double)pBytes / totalBytes * availW));
        if (i == m_partitions.size() - 1) w = r.right() - curX;

        QRect partRect(curX, r.top(), w, r.height());
        if (partRect.contains(event->pos())) {
            newHover = i;
            break;
        }
        curX += w;
    }

    if (newHover != m_hoveredIndex) {
        m_hoveredIndex = newHover;
        update();
    }
}

void PartitionMapWidget::leaveEvent(QEvent*)
{
    m_hoveredIndex = -1;
    update();
}

// ==========================================
// DiskImagePage Main Format Page
// ==========================================
DiskImagePage::DiskImagePage(QWidget *parent)
    : FormatPage(parent)
{
    setObjectName("DiskImagePage");
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // Header bar
    auto* header = new QHBoxLayout();
    header->setSpacing(10);

    auto* title = new QLabel(tr("<b>Просмотрщик образов дисков и файловых систем</b>"), this);
    title->setStyleSheet("font-size: 15px; color: #f3eefc; font-weight: bold;");
    header->addWidget(title);
    header->addStretch(1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #c084fc; font-size: 11px;");
    header->addWidget(m_statusLabel);

    m_openImageBtn = new QPushButton(tr("📂 Выбрать другой .img / .iso..."), this);
    m_openImageBtn->setCursor(Qt::PointingHandCursor);
    m_openImageBtn->setStyleSheet(
        "QPushButton { background: #261a38; color: #d8b4fe; border: 1px solid #6b21a8; "
        "border-radius: 6px; padding: 5px 12px; font-weight: 500; }"
        "QPushButton:hover { background: #3c1e5c; color: #ffffff; border-color: #a855f7; }");
    connect(m_openImageBtn, &QPushButton::clicked, this, &DiskImagePage::onOpenAnotherImageClicked);
    header->addWidget(m_openImageBtn);

    mainLayout->addLayout(header);

    // Graphical Partition Map
    m_partitionMap = new PartitionMapWidget(this);
    connect(m_partitionMap, &PartitionMapWidget::partitionClicked, this, &DiskImagePage::onPartitionSelected);
    mainLayout->addWidget(m_partitionMap);

    // Main Splitter: Left = Partitions list & Details, Right = File Browser
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // Left Pane: Partitions & Metadata
    auto* leftPane = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* partListTitle = new QLabel(tr("Разделы диска:"), leftPane);
    partListTitle->setStyleSheet("font-weight: bold; color: #c4b5fd; font-size: 12px;");
    leftLayout->addWidget(partListTitle);

    m_partitionsTable = new QTableWidget(0, 4, leftPane);
    m_partitionsTable->setHorizontalHeaderLabels({tr("№"), tr("Файловая система"), tr("Размер"), tr("Смещение")});
    m_partitionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_partitionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partitionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_partitionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partitionsTable->verticalHeader()->setVisible(false);
    connect(m_partitionsTable, &QTableWidget::cellClicked, this, [this](int row, int) {
        onPartitionSelected(row);
    });
    leftLayout->addWidget(m_partitionsTable, 1);

    auto* detailsBox = new QGroupBox(tr("Свойства суперблока"), leftPane);
    auto* detailsLayout = new QVBoxLayout(detailsBox);
    m_fsDetailsLabel = new QLabel(tr("Выберите раздел для просмотра свойств"), detailsBox);
    m_fsDetailsLabel->setWordWrap(true);
    m_fsDetailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_fsDetailsLabel->setStyleSheet("color: #d1c5e4; font-size: 11px;");
    detailsLayout->addWidget(m_fsDetailsLabel);
    leftLayout->addWidget(detailsBox);

    splitter->addWidget(leftPane);

    // Right Pane: Files Browser
    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* filesTopRow = new QHBoxLayout();
    m_currentPathLabel = new QLabel(tr("Содержимое раздела /"), rightPane);
    m_currentPathLabel->setStyleSheet("font-weight: bold; color: #f3eefc; font-size: 12px;");
    filesTopRow->addWidget(m_currentPathLabel);
    filesTopRow->addStretch(1);

    m_filterEdit = new QLineEdit(rightPane);
    m_filterEdit->setPlaceholderText(tr("Фильтр файлов..."));
    m_filterEdit->setMaximumWidth(200);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &DiskImagePage::onFilterChanged);
    filesTopRow->addWidget(m_filterEdit);

    rightLayout->addLayout(filesTopRow);

    m_filesTable = new QTableWidget(0, 4, rightPane);
    m_filesTable->setHorizontalHeaderLabels({tr("Имя"), tr("Размер"), tr("Тип"), tr("Права / Inode")});
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_filesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_filesTable->verticalHeader()->setVisible(false);
    connect(m_filesTable, &QTableWidget::itemDoubleClicked, this, &DiskImagePage::onFileItemDoubleClicked);
    rightLayout->addWidget(m_filesTable, 1);

    // Actions under file list
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_extractBtn = new QPushButton(tr("📥 Извлечь файл..."), rightPane);
    m_extractBtn->setCursor(Qt::PointingHandCursor);
    m_extractBtn->setStyleSheet(
        "QPushButton { background: #7c3aed; color: #ffffff; border: none; "
        "border-radius: 5px; padding: 6px 14px; font-weight: bold; }"
        "QPushButton:hover { background: #9055ff; }");
    connect(m_extractBtn, &QPushButton::clicked, this, &DiskImagePage::onExtractClicked);
    btnRow->addWidget(m_extractBtn);

    m_openHexBtn = new QPushButton(tr("👁 Открыть в HEX-редакторе"), rightPane);
    m_openHexBtn->setCursor(Qt::PointingHandCursor);
    m_openHexBtn->setStyleSheet(
        "QPushButton { background: #261a38; color: #c084fc; border: 1px solid #6b21a8; "
        "border-radius: 5px; padding: 6px 14px; font-weight: 500; }"
        "QPushButton:hover { background: #3b2060; border-color: #a855f7; color: #ffffff; }");
    connect(m_openHexBtn, &QPushButton::clicked, this, &DiskImagePage::onOpenInHexClicked);
    btnRow->addWidget(m_openHexBtn);

    btnRow->addStretch(1);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);
}

void DiskImagePage::setPageData(QByteArray& data)
{
    m_currentData = data;
    m_loadedFilePath.clear();
    disk::DiskImageParser::parseImage(data, m_partitions);
    populatePartitions();
    emit dataEqual();
}

QByteArray DiskImagePage::getPageData() const
{
    return QByteArray();
}

void DiskImagePage::loadExternalImage(const QString& filePath)
{
    m_loadedFilePath = filePath;
    disk::DiskImageParser::parseImageFile(filePath, m_partitions);
    populatePartitions();
}

void DiskImagePage::onOpenAnotherImageClicked()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        tr("Открыть образ диска"),
        QString(),
        tr("Образы дисков (*.img *.iso *.bin *.raw *.vmdk *.qcow2);;Все файлы (*.*)")
    );
    if (!file.isEmpty()) {
        loadExternalImage(file);
    }
}

void DiskImagePage::populatePartitions()
{
    m_partitionMap->setPartitions(m_partitions);
    m_partitionsTable->setRowCount(0);

    if (m_partitions.isEmpty()) {
        m_statusLabel->setText(tr("Файловая система не распознана"));
        m_filesTable->setRowCount(0);
        m_fsDetailsLabel->setText(tr("Образ пуст или формат не поддерживается"));
        return;
    }

    m_statusLabel->setText(tr("Обнаружено разделов: %1").arg(m_partitions.size()));

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
        auto* offItem  = new QTableWidgetItem(QString("0x%1").arg(part.startByte, 0, 16));

        m_partitionsTable->setItem(i, 0, numItem);
        m_partitionsTable->setItem(i, 1, fsItem);
        m_partitionsTable->setItem(i, 2, sizeItem);
        m_partitionsTable->setItem(i, 3, offItem);
    }

    m_partitionsTable->selectRow(0);
    onPartitionSelected(0);
}

void DiskImagePage::onPartitionSelected(int index)
{
    if (index < 0 || index >= m_partitions.size()) return;
    m_activePartitionIndex = index;
    m_partitionMap->setSelectedPartition(index);
    m_partitionsTable->selectRow(index);

    const auto& part = m_partitions[index];
    updateDetails(part);
    populateFiles(part);
}

void DiskImagePage::updateDetails(const disk::PartitionInfo& part)
{
    QString info = QString("<b>Раздел:</b> %1<br>"
                           "<b>Файловая система:</b> <span style='color:#c084fc;'>%2</span><br>"
                           "<b>Метка:</b> %3<br>"
                           "<b>Размер:</b> %4 байт<br>"
                           "<b>Начальный сектор:</b> %5<br>")
                       .arg(part.name)
                       .arg(part.fsType)
                       .arg(part.label.isEmpty() ? tr("(нет)") : part.label)
                       .arg(part.sizeBytes)
                       .arg(part.startSector);

    for (auto it = part.details.constBegin(); it != part.details.constEnd(); ++it) {
        info += QString("<b>%1:</b> %2<br>").arg(it.key()).arg(it.value());
    }

    m_fsDetailsLabel->setText(info);
}

void DiskImagePage::populateFiles(const disk::PartitionInfo& part)
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

void DiskImagePage::onFilterChanged(const QString&)
{
    if (m_activePartitionIndex >= 0 && m_activePartitionIndex < m_partitions.size()) {
        populateFiles(m_partitions[m_activePartitionIndex]);
    }
}

void DiskImagePage::onFileItemDoubleClicked(QTableWidgetItem* item)
{
    if (!item) return;
    int row = item->row();
    auto* nameItem = m_filesTable->item(row, 0);
    if (!nameItem) return;

    QString fileName = nameItem->text();
    if (fileName.startsWith("📁 ")) {
        QMessageBox::information(this, tr("Папка"), tr("Переход в подпапку: %1").arg(fileName.mid(3)));
    } else {
        onOpenInHexClicked();
    }
}

void DiskImagePage::onExtractClicked()
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

    QByteArray extracted;
    if (!m_loadedFilePath.isEmpty()) {
        extracted = disk::DiskImageParser::extractFileData(m_loadedFilePath, part, targetEntry);
    } else {
        extracted = disk::DiskImageParser::extractFileDataFromBuffer(m_currentData, part, targetEntry);
    }

    QFile outFile(destFile);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(extracted);
        outFile.close();
        QMessageBox::information(this, tr("Успех"), tr("Файл успешно извлечён в:\n%1").arg(destFile));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось записать файл."));
    }
}

void DiskImagePage::onOpenInHexClicked()
{
    int row = m_filesTable->currentRow();
    if (row < 0 || m_activePartitionIndex >= m_partitions.size()) return;

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

    if (targetEntry.dataOffset > 0 && targetEntry.size > 0) {
        emit selectionChanged(targetEntry.dataOffset, targetEntry.size);
        emit statusBarInfoChanged(tr("Файл: %1 (Смещение: 0x%2, Размер: %3)")
                                      .arg(targetEntry.name)
                                      .arg(targetEntry.dataOffset, 0, 16)
                                      .arg(targetEntry.size));
    } else {
        QMessageBox::information(this, tr("Файл"), tr("Выбран файл: %1 (%2 байт)").arg(targetEntry.name).arg(targetEntry.size));
    }
}
