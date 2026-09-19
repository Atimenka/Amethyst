/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#ifndef DISK_IMAGE_PARSER_H
#define DISK_IMAGE_PARSER_H

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>
#include <QVector>

namespace disk {

struct FsEntry {
    QString name;
    QString fullPath;
    quint64 size = 0;
    bool isDirectory = false;
    bool isSymlink = false;
    QString permissions;
    QDateTime modified;
    quint64 inode = 0;
    quint64 dataOffset = 0;
    QByteArray inlineData;
    QList<FsEntry> children;
};

struct PartitionInfo {
    int index = 0;
    QString name;
    quint64 startSector = 0;
    quint64 sectorCount = 0;
    quint64 startByte = 0;
    quint64 sizeBytes = 0;
    QString fsType;       // "ext4", "ext3", "ext2", "ext1", "NTFS", "EROFS", "Btrfs", "FAT32", "FAT16", "ISO9660", "SquashFS", "RAW"
    QString label;
    QString uuid;
    bool bootable = false;
    quint32 blockSize = 512;
    QMap<QString, QString> details;
    QList<FsEntry> rootEntries;
    bool parsedSuccessfully = false;
};

class DiskImageParser {
public:
    DiskImageParser() = default;

    static bool parseImage(const QByteArray& data, QVector<PartitionInfo>& outPartitions);
    static bool parseImageFile(const QString& filePath, QVector<PartitionInfo>& outPartitions);
    static QByteArray extractFileData(const QString& imageFilePath, const PartitionInfo& part, const FsEntry& entry);
    static QByteArray extractFileDataFromBuffer(const QByteArray& data, const PartitionInfo& part, const FsEntry& entry);

    static bool detectAndParseFs(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);

private:
    static bool parseMbr(const QByteArray& data, QVector<PartitionInfo>& partitions);
    static bool parseGpt(const QByteArray& data, QVector<PartitionInfo>& partitions);

    // Specific filesystem parsers
    static bool parseExt(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);
    static bool parseNtfs(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);
    static bool parseErofs(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);
    static bool parseBtrfs(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);
    static bool parseFat(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);
    static bool parseIso9660(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);
    static bool parseSquashfs(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part);

    static bool parseWithSystemTools(const QString& filePath, QVector<PartitionInfo>& partitions);
};

} // namespace disk

#endif // DISK_IMAGE_PARSER_H
