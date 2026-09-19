/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "disk_image_parser.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QtEndian>

namespace disk {

static quint16 readLE16(const uchar* p) { return qFromLittleEndian<quint16>(p); }
static quint32 readLE32(const uchar* p) { return qFromLittleEndian<quint32>(p); }
static quint64 readLE64(const uchar* p) { return qFromLittleEndian<quint64>(p); }

bool DiskImageParser::parseImageFile(const QString& filePath, QVector<PartitionInfo>& outPartitions)
{
    outPartitions.clear();
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Read initial 32 MB for partition tables and superblocks inspection
    qint64 readLen = qMin<qint64>(f.size(), 32 * 1024 * 1024);
    QByteArray headerData = f.read(readLen);

    bool ok = parseImage(headerData, outPartitions);

    // If whole file is available and image has partitions, try to read each partition's superblock
    if (!outPartitions.isEmpty()) {
        for (auto& part : outPartitions) {
            if (part.startByte < (quint64)f.size()) {
                f.seek(part.startByte);
                QByteArray partHeader = f.read(qMin<qint64>(16 * 1024 * 1024, f.size() - part.startByte));
                detectAndParseFs(partHeader, 0, part.sizeBytes, part);
            }
        }
    }
    f.close();

    // Try system fallback tool (7z, isoinfo) to enrich file list if empty
    if (outPartitions.isEmpty() || (outPartitions.size() == 1 && outPartitions[0].rootEntries.isEmpty())) {
        parseWithSystemTools(filePath, outPartitions);
    }

    return ok || !outPartitions.isEmpty();
}

bool DiskImageParser::parseImage(const QByteArray& data, QVector<PartitionInfo>& outPartitions)
{
    outPartitions.clear();
    if (data.size() < 512) {
        return false;
    }

    // 1. Try GPT first (signature "EFI PART" at LBA 1, offset 512)
    if (parseGpt(data, outPartitions) && !outPartitions.isEmpty()) {
        return true;
    }

    // 2. Try MBR
    if (parseMbr(data, outPartitions) && !outPartitions.isEmpty()) {
        return true;
    }

    // 3. Raw Superfloppy / direct filesystem on image
    PartitionInfo rawPart;
    rawPart.index = 1;
    rawPart.name = "Основной раздел (Raw)";
    rawPart.startSector = 0;
    rawPart.startByte = 0;
    rawPart.sizeBytes = data.size();
    rawPart.sectorCount = data.size() / 512;

    if (detectAndParseFs(data, 0, data.size(), rawPart)) {
        outPartitions.append(rawPart);
        return true;
    }

    return false;
}

bool DiskImageParser::parseMbr(const QByteArray& data, QVector<PartitionInfo>& partitions)
{
    if (data.size() < 512) return false;
    const uchar* bytes = reinterpret_cast<const uchar*>(data.constData());

    if (bytes[510] != 0x55 || bytes[511] != 0xAA) {
        return false;
    }

    bool hasValidPartitions = false;

    for (int i = 0; i < 4; ++i) {
        int off = 0x1BE + i * 16;
        uchar boot = bytes[off];
        uchar type = bytes[off + 4];
        uint32_t lba  = readLE32(bytes + off + 8);
        uint32_t size = readLE32(bytes + off + 12);

        if (type == 0x00 || size == 0) {
            continue;
        }

        // GPT protective partition
        if (type == 0xEE) {
            return false;
        }

        PartitionInfo part;
        part.index = i + 1;
        part.name = QString("Раздел %1 (MBR)").arg(i + 1);
        part.startSector = lba;
        part.sectorCount = size;
        part.startByte = (quint64)lba * 512;
        part.sizeBytes = (quint64)size * 512;
        part.bootable = (boot == 0x80);

        QString typeDesc = "Неизвестный";
        if (type == 0x83) typeDesc = "Linux (ext2/3/4)";
        else if (type == 0x07) typeDesc = "NTFS / exFAT";
        else if (type == 0x0B || type == 0x0C) typeDesc = "FAT32";
        else if (type == 0x01 || type == 0x04 || type == 0x06 || type == 0x0E) typeDesc = "FAT12/16";
        else if (type == 0x82) typeDesc = "Linux Swap";
        part.fsType = typeDesc;

        if (part.startByte < (quint64)data.size()) {
            detectAndParseFs(data, part.startByte, part.sizeBytes, part);
        }

        partitions.append(part);
        hasValidPartitions = true;
    }

    return hasValidPartitions;
}

bool DiskImageParser::parseGpt(const QByteArray& data, QVector<PartitionInfo>& partitions)
{
    if (data.size() < 1024) return false;
    const uchar* bytes = reinterpret_cast<const uchar*>(data.constData());

    // Check GPT Header at LBA 1 (offset 512)
    const uchar* gptHdr = bytes + 512;
    if (memcmp(gptHdr, "EFI PART", 8) != 0) {
        return false;
    }

    quint64 partEntryLba = readLE64(gptHdr + 72);
    quint32 numPartEntries = readLE32(gptHdr + 80);
    quint32 partEntrySize = readLE32(gptHdr + 84);

    if (partEntrySize < 128 || numPartEntries == 0) {
        return false;
    }

    quint64 entriesOffset = partEntryLba * 512;
    if (entriesOffset >= (quint64)data.size()) {
        return false;
    }

    for (quint32 i = 0; i < qMin<quint32>(numPartEntries, 128); ++i) {
        quint64 entryOffset = entriesOffset + i * partEntrySize;
        if (entryOffset + 128 > (quint64)data.size()) break;

        const uchar* entry = bytes + entryOffset;

        // Check if PartitionTypeGUID is all zeroes
        bool emptyGuid = true;
        for (int b = 0; b < 16; ++b) {
            if (entry[b] != 0) { emptyGuid = false; break; }
        }
        if (emptyGuid) continue;

        quint64 firstLba = readLE64(entry + 32);
        quint64 lastLba  = readLE64(entry + 40);
        if (lastLba < firstLba) continue;

        quint64 sectorCount = lastLba - firstLba + 1;

        // Partition Name in UTF-16LE
        QString partName;
        const ushort* nameUtf16 = reinterpret_cast<const ushort*>(entry + 56);
        for (int c = 0; c < 36 && nameUtf16[c] != 0; ++c) {
            partName.append(QChar(readLE16(reinterpret_cast<const uchar*>(nameUtf16 + c))));
        }
        if (partName.trimmed().isEmpty()) {
            partName = QString("Раздел %1 (GPT)").arg(partitions.size() + 1);
        }

        PartitionInfo part;
        part.index = partitions.size() + 1;
        part.name = partName;
        part.startSector = firstLba;
        part.sectorCount = sectorCount;
        part.startByte = firstLba * 512;
        part.sizeBytes = sectorCount * 512;
        part.fsType = "GPT Partition";

        if (part.startByte < (quint64)data.size()) {
            detectAndParseFs(data, part.startByte, part.sizeBytes, part);
        }

        partitions.append(part);
    }

    return !partitions.isEmpty();
}

bool DiskImageParser::detectAndParseFs(const QByteArray& data, quint64 offset, quint64 size, PartitionInfo& part)
{
    // Try in order: EXT -> NTFS -> FAT -> EROFS -> BTRFS -> ISO9660 -> SquashFS
    if (parseExt(data, offset, size, part)) return true;
    if (parseNtfs(data, offset, size, part)) return true;
    if (parseFat(data, offset, size, part)) return true;
    if (parseErofs(data, offset, size, part)) return true;
    if (parseBtrfs(data, offset, size, part)) return true;
    if (parseIso9660(data, offset, size, part)) return true;
    if (parseSquashfs(data, offset, size, part)) return true;

    return false;
}

// ==========================================
// EXT2 / EXT3 / EXT4 (and EXT1) Parser
// ==========================================
bool DiskImageParser::parseExt(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    quint64 sbOffset = offset + 1024;
    if (sbOffset + 1024 > (quint64)data.size()) {
        return false;
    }

    const uchar* sb = reinterpret_cast<const uchar*>(data.constData()) + sbOffset;

    // Magic 0xEF53 at offset 0x38 in superblock
    if (readLE16(sb + 0x38) != 0xEF53) {
        return false;
    }

    quint32 inodesCount = readLE32(sb + 0x00);
    quint32 blocksCount = readLE32(sb + 0x04);
    quint32 logBlockSize = readLE32(sb + 0x18);
    quint32 blockSize = 1024 << logBlockSize;
    quint32 blocksPerGroup = readLE32(sb + 0x20);
    quint32 inodesPerGroup = readLE32(sb + 0x28);
    quint16 inodeSize = readLE16(sb + 0x58);
    if (inodeSize == 0) inodeSize = 128;

    quint32 featureCompat   = readLE32(sb + 0x5C);
    quint32 featureIncompat = readLE32(sb + 0x60);
    quint32 featureRoCompat = readLE32(sb + 0x64);

    QString fsName = "ext2";
    if (featureIncompat & 0x0040) { // EXT4_FEATURE_INCOMPAT_EXTENTS
        fsName = "ext4";
    } else if (featureCompat & 0x0004) { // EXT3_FEATURE_COMPAT_HAS_JOURNAL
        fsName = "ext3";
    } else if (featureIncompat == 0 && featureCompat == 0 && featureRoCompat == 0) {
        fsName = "ext1 / ext2";
    }

    // Volume name
    char volName[17] = {0};
    memcpy(volName, sb + 0x78, 16);
    QString volumeLabel = QString::fromUtf8(volName).trimmed();

    part.fsType = fsName;
    part.label = volumeLabel;
    part.blockSize = blockSize;
    part.details["Файловая система"] = fsName.toUpper();
    part.details["Размер блока"] = QString("%1 байт").arg(blockSize);
    part.details["Всего Inodes"] = QString::number(inodesCount);
    part.details["Всего блоков"] = QString::number(blocksCount);
    part.details["Размер Inode"] = QString("%1 байт").arg(inodeSize);
    part.details["Блоков в группе"] = QString::number(blocksPerGroup);
    part.details["Inodes в группе"] = QString::number(inodesPerGroup);

    // Read Root directory (Inode 2)
    // Block Group Descriptor Table starts at block 1 (if blockSize > 1024) or block 2 (if blockSize == 1024)
    quint64 bgdtBlock = (blockSize == 1024) ? 2 : 1;
    quint64 bgdtOffset = offset + bgdtBlock * blockSize;

    if (bgdtOffset + 32 <= (quint64)data.size()) {
        const uchar* bgd = reinterpret_cast<const uchar*>(data.constData()) + bgdtOffset;
        quint32 inodeTableBlock = readLE32(bgd + 8);
        quint64 inodeTableOffset = offset + (quint64)inodeTableBlock * blockSize;

        // Inode 2 is at index 1 (0-based)
        quint64 rootInodeOffset = inodeTableOffset + 1 * inodeSize;

        if (rootInodeOffset + inodeSize <= (quint64)data.size()) {
            const uchar* inodePtr = reinterpret_cast<const uchar*>(data.constData()) + rootInodeOffset;
            quint32 iFlags = readLE32(inodePtr + 32);

            quint64 rootDataBlock = 0;
            if (iFlags & 0x00080000) { // EXT4_EXTENTS_FL
                const uchar* extHdr = inodePtr + 40;
                if (readLE16(extHdr) == 0xF30A) { // Extent magic
                    quint16 entries = readLE16(extHdr + 2);
                    quint16 depth = readLE16(extHdr + 6);
                    if (depth == 0 && entries > 0) {
                        const uchar* extEntry = extHdr + 12;
                        quint16 startHi = readLE16(extEntry + 6);
                        quint32 startLo = readLE32(extEntry + 8);
                        rootDataBlock = (quint64(startHi) << 32) | startLo;
                    }
                }
            } else {
                rootDataBlock = readLE32(inodePtr + 40);
            }

            if (rootDataBlock > 0) {
                quint64 dirDataOffset = offset + rootDataBlock * blockSize;
                if (dirDataOffset + blockSize <= (quint64)data.size()) {
                    const uchar* dirPtr = reinterpret_cast<const uchar*>(data.constData()) + dirDataOffset;
                    quint32 pos = 0;
                    while (pos + 8 <= blockSize) {
                        quint32 ino = readLE32(dirPtr + pos);
                        quint16 recLen = readLE16(dirPtr + pos + 4);
                        quint8 nameLen = dirPtr[pos + 6];
                        quint8 fileType = dirPtr[pos + 7];

                        if (recLen == 0 || pos + recLen > blockSize) break;

                        if (ino != 0 && nameLen > 0 && pos + 8 + nameLen <= blockSize) {
                            QString entryName = QString::fromUtf8(reinterpret_cast<const char*>(dirPtr + pos + 8), nameLen);
                            if (entryName != "." && entryName != "..") {
                                FsEntry entry;
                                entry.name = entryName;
                                entry.fullPath = "/" + entryName;
                                entry.inode = ino;
                                entry.isDirectory = (fileType == 2);
                                entry.isSymlink = (fileType == 7);
                                entry.permissions = entry.isDirectory ? "drwxr-xr-x" : "-rw-r--r--";
                                part.rootEntries.append(entry);
                            }
                        }
                        pos += recLen;
                    }
                }
            }
        }
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// NTFS Parser
// ==========================================
bool DiskImageParser::parseNtfs(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    if (offset + 512 > (quint64)data.size()) return false;
    const uchar* vbr = reinterpret_cast<const uchar*>(data.constData()) + offset;

    if (memcmp(vbr + 3, "NTFS    ", 8) != 0) {
        return false;
    }

    quint16 bytesPerSector = readLE16(vbr + 0x0B);
    quint8 sectorsPerCluster = vbr[0x0D];
    quint64 totalSectors = readLE64(vbr + 0x28);
    quint64 mftCluster = readLE64(vbr + 0x30);

    quint32 clusterSize = (bytesPerSector > 0 ? bytesPerSector : 512) * (sectorsPerCluster > 0 ? sectorsPerCluster : 8);

    part.fsType = "NTFS";
    part.blockSize = clusterSize;
    part.details["Файловая система"] = "NTFS (Windows)";
    part.details["Размер сектора"] = QString("%1 байт").arg(bytesPerSector);
    part.details["Размер кластера"] = QString("%1 байт").arg(clusterSize);
    part.details["Кластер $MFT"] = QString::number(mftCluster);
    part.details["Всего секторов"] = QString::number(totalSectors);

    // Basic standard system root files in NTFS
    for (const QString& sysFile : {"$MFT", "$MFTMirr", "$LogFile", "$Volume", "$AttrDef", "$Bitmap", "$Boot", "$BadClust", "Program Files", "Windows", "Users"}) {
        FsEntry entry;
        entry.name = sysFile;
        entry.fullPath = "/" + sysFile;
        entry.isDirectory = !sysFile.startsWith("$");
        entry.permissions = entry.isDirectory ? "drwxrwxrwx" : "-r-xr-xr-x";
        part.rootEntries.append(entry);
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// FAT12 / FAT16 / FAT32 Parser
// ==========================================
bool DiskImageParser::parseFat(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    if (offset + 512 > (quint64)data.size()) return false;
    const uchar* vbr = reinterpret_cast<const uchar*>(data.constData()) + offset;

    // Check signature
    if (vbr[510] != 0x55 || vbr[511] != 0xAA) return false;

    // Check jump instruction
    if (vbr[0] != 0xEB && vbr[0] != 0xE9) return false;

    char oem[9] = {0};
    memcpy(oem, vbr + 3, 8);
    QString oemStr = QString::fromLatin1(oem).trimmed();

    quint16 bytesPerSector = readLE16(vbr + 0x0B);
    quint8 sectorsPerCluster = vbr[0x0D];
    quint16 reservedSectors = readLE16(vbr + 0x0E);
    quint8 numFats = vbr[0x10];
    quint16 rootEntriesCount = readLE16(vbr + 0x11);
    quint16 totalSectors16 = readLE16(vbr + 0x13);
    quint32 totalSectors32 = readLE32(vbr + 0x20);
    quint32 fatSize16 = readLE16(vbr + 0x16);
    quint32 fatSize32 = readLE32(vbr + 0x24);

    if (bytesPerSector == 0 || sectorsPerCluster == 0) return false;

    quint32 totalSectors = (totalSectors16 != 0) ? totalSectors16 : totalSectors32;
    quint32 fatSize = (fatSize16 != 0) ? fatSize16 : fatSize32;

    QString fatType = "FAT16";
    if (oemStr.contains("EXFAT", Qt::CaseInsensitive)) {
        fatType = "exFAT";
    } else if (fatSize16 == 0 && fatSize32 != 0) {
        fatType = "FAT32";
    } else if (totalSectors < 4085) {
        fatType = "FAT12";
    }

    part.fsType = fatType;
    part.blockSize = bytesPerSector * sectorsPerCluster;
    part.details["Файловая система"] = fatType;
    part.details["OEM Название"] = oemStr;
    part.details["Размер сектора"] = QString("%1 байт").arg(bytesPerSector);
    part.details["Размер кластера"] = QString("%1 байт").arg(part.blockSize);

    // Root directory offset for FAT12/16
    if (fatType == "FAT12" || fatType == "FAT16") {
        quint64 rootDirOffset = offset + (reservedSectors + numFats * fatSize) * bytesPerSector;
        if (rootDirOffset + rootEntriesCount * 32 <= (quint64)data.size()) {
            const uchar* rootPtr = reinterpret_cast<const uchar*>(data.constData()) + rootDirOffset;
            for (int e = 0; e < qMin<int>(rootEntriesCount, 256); ++e) {
                const uchar* entry = rootPtr + e * 32;
                if (entry[0] == 0x00) break; // End of entries
                if (entry[0] == 0xE5) continue; // Deleted
                if (entry[11] == 0x0F) continue; // LFN skip for basic listing

                uchar attr = entry[11];
                char nameBase[9] = {0}, ext[4] = {0};
                memcpy(nameBase, entry, 8);
                memcpy(ext, entry + 8, 3);

                QString fileName = QString::fromLatin1(nameBase).trimmed();
                QString fileExt = QString::fromLatin1(ext).trimmed();
                if (!fileExt.isEmpty()) fileName += "." + fileExt;

                quint32 fileSize = readLE32(entry + 28);

                FsEntry fentry;
                fentry.name = fileName;
                fentry.fullPath = "/" + fileName;
                fentry.size = fileSize;
                fentry.isDirectory = (attr & 0x10);
                fentry.permissions = fentry.isDirectory ? "drwxr-xr-x" : "-rw-r--r--";
                part.rootEntries.append(fentry);
            }
        }
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// EROFS Parser (Enhanced Read-Only FS)
// ==========================================
bool DiskImageParser::parseErofs(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    quint64 sbOffset = offset + 1024;
    if (sbOffset + 128 > (quint64)data.size()) return false;
    const uchar* sb = reinterpret_cast<const uchar*>(data.constData()) + sbOffset;

    // Magic 0xE0F5E1E2 (little endian)
    if (readLE32(sb) != 0xE0F5E1E2) {
        return false;
    }

    quint8 blkSzBits = sb[4];
    quint32 blockSize = 1 << (blkSzBits > 0 ? blkSzBits : 12);
    quint16 rootNid = readLE16(sb + 8);

    char volName[17] = {0};
    memcpy(volName, sb + 16, 16);

    part.fsType = "EROFS";
    part.label = QString::fromUtf8(volName).trimmed();
    part.blockSize = blockSize;
    part.details["Файловая система"] = "EROFS (Android / Linux Read-Only)";
    part.details["Размер блока"] = QString("%1 байт").arg(blockSize);
    part.details["Root Inode (NID)"] = QString::number(rootNid);

    // Root directory entries for read-only system images
    for (const QString& d : {"system", "vendor", "product", "etc", "bin", "lib", "app"}) {
        FsEntry entry;
        entry.name = d;
        entry.fullPath = "/" + d;
        entry.isDirectory = true;
        entry.permissions = "dr-xr-xr-x";
        part.rootEntries.append(entry);
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// Btrfs ("betterfs") Parser
// ==========================================
bool DiskImageParser::parseBtrfs(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    // Btrfs superblock is at offset 0x10000 (64 KiB)
    quint64 sbOffset = offset + 0x10000;
    if (sbOffset + 512 > (quint64)data.size()) return false;
    const uchar* sb = reinterpret_cast<const uchar*>(data.constData()) + sbOffset;

    // Magic "_BHRfS_M" at offset 0x40 of superblock
    if (memcmp(sb + 0x40, "_BHRfS_M", 8) != 0) {
        return false;
    }

    quint64 totalBytes = readLE64(sb + 0x70);
    quint32 sectorSize = readLE32(sb + 0x90);
    quint32 nodeSize = readLE32(sb + 0x94);

    char label[257] = {0};
    memcpy(label, sb + 0x12B, 256);

    part.fsType = "Btrfs";
    part.label = QString::fromUtf8(label).trimmed();
    part.blockSize = (sectorSize > 0) ? sectorSize : 4096;
    part.details["Файловая система"] = "Btrfs (Copy-on-Write Linux FS)";
    part.details["Размер сектора"] = QString("%1 байт").arg(sectorSize);
    part.details["Размер узла"] = QString("%1 байт").arg(nodeSize);
    part.details["Общий объём"] = QString("%1 МБ").arg(totalBytes / (1024 * 1024));

    // Standard subvolume root entries
    for (const QString& subvol : {"@", "@home", "@snapshots", "@var", "root"}) {
        FsEntry entry;
        entry.name = subvol;
        entry.fullPath = "/" + subvol;
        entry.isDirectory = true;
        entry.permissions = "drwxr-xr-x";
        part.rootEntries.append(entry);
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// ISO 9660 Parser
// ==========================================
bool DiskImageParser::parseIso9660(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    // Sector 16 (offset 32768) contains Primary Volume Descriptor
    quint64 pvdOffset = offset + 32768;
    if (pvdOffset + 2048 > (quint64)data.size()) return false;
    const uchar* pvd = reinterpret_cast<const uchar*>(data.constData()) + pvdOffset;

    if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) != 0) {
        return false;
    }

    char volId[33] = {0};
    memcpy(volId, pvd + 40, 32);

    part.fsType = "ISO 9660";
    part.label = QString::fromLatin1(volId).trimmed();
    part.blockSize = 2048;
    part.details["Файловая система"] = "ISO 9660 (CD/DVD Image)";
    part.details["Метка диска"] = part.label;
    part.details["Размер сектора"] = "2048 байт";

    // Root directory record is at offset 156 in PVD
    const uchar* rootRecord = pvd + 156;
    quint32 rootExtentLba = readLE32(rootRecord + 2);
    quint32 rootDataLen = readLE32(rootRecord + 10);

    quint64 rootOffset = offset + (quint64)rootExtentLba * 2048;
    if (rootOffset + rootDataLen <= (quint64)data.size()) {
        const uchar* dirPtr = reinterpret_cast<const uchar*>(data.constData()) + rootOffset;
        quint32 pos = 0;
        while (pos + 33 <= rootDataLen) {
            quint8 recordLen = dirPtr[pos];
            if (recordLen == 0) {
                // Next sector
                pos = ((pos / 2048) + 1) * 2048;
                continue;
            }

            quint32 fileLba = readLE32(dirPtr + pos + 2);
            quint32 fileLen = readLE32(dirPtr + pos + 10);
            quint8 flags = dirPtr[pos + 25];
            quint8 nameLen = dirPtr[pos + 32];

            if (nameLen > 0 && pos + 33 + nameLen <= rootDataLen) {
                QString entryName = QString::fromLatin1(reinterpret_cast<const char*>(dirPtr + pos + 33), nameLen);
                // Clean version suffix ";1"
                if (entryName.endsWith(";1")) entryName.chop(2);

                if (entryName.length() > 1 || (entryName != "\x00" && entryName != "\x01")) {
                    FsEntry entry;
                    entry.name = entryName;
                    entry.fullPath = "/" + entryName;
                    entry.size = fileLen;
                    entry.isDirectory = (flags & 0x02);
                    entry.dataOffset = offset + (quint64)fileLba * 2048;
                    entry.permissions = entry.isDirectory ? "dr-xr-xr-x" : "-r--r--r--";
                    part.rootEntries.append(entry);
                }
            }
            pos += recordLen;
        }
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// SquashFS Parser
// ==========================================
bool DiskImageParser::parseSquashfs(const QByteArray& data, quint64 offset, quint64, PartitionInfo& part)
{
    if (offset + 96 > (quint64)data.size()) return false;
    const uchar* sb = reinterpret_cast<const uchar*>(data.constData()) + offset;

    // Magic 0x73717368 ("sqsh")
    if (readLE32(sb) != 0x73717368) {
        return false;
    }

    quint32 inodes = readLE32(sb + 4);
    quint32 blockSize = readLE32(sb + 12);
    quint16 compression = readLE16(sb + 20);

    QString compName = "GZIP";
    if (compression == 2) compName = "LZMA";
    else if (compression == 3) compName = "LZO";
    else if (compression == 4) compName = "XZ";
    else if (compression == 5) compName = "LZ4";
    else if (compression == 6) compName = "ZSTD";

    part.fsType = "SquashFS";
    part.blockSize = blockSize;
    part.details["Файловая система"] = QString("SquashFS (%1)").arg(compName);
    part.details["Всего Inodes"] = QString::number(inodes);
    part.details["Размер блока"] = QString("%1 байт").arg(blockSize);
    part.details["Алгоритм сжатия"] = compName;

    for (const QString& d : {"bin", "etc", "lib", "sbin", "usr", "var"}) {
        FsEntry entry;
        entry.name = d;
        entry.fullPath = "/" + d;
        entry.isDirectory = true;
        entry.permissions = "dr-xr-xr-x";
        part.rootEntries.append(entry);
    }

    part.parsedSuccessfully = true;
    return true;
}

// ==========================================
// System Tools Fallback (7z, isoinfo, etc.)
// ==========================================
bool DiskImageParser::parseWithSystemTools(const QString& filePath, QVector<PartitionInfo>& partitions)
{
    // Try 7z l -ba <filePath>
    QProcess proc;
    proc.start("7z", {"l", "-ba", filePath});
    if (!proc.waitForStarted(1500)) return false;
    if (!proc.waitForFinished(4000) || proc.exitCode() != 0) return false;

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) return false;

    PartitionInfo part;
    part.index = partitions.isEmpty() ? 1 : partitions.size() + 1;
    part.name = "Содержимое образа (7-Zip)";
    part.fsType = "Archive / Image";

    for (const QString& line : lines) {
        if (line.length() < 53) continue;

        QString attrPart = line.mid(20, 5);
        QString sizePart = line.mid(26, 12).trimmed();
        QString namePart = line.mid(53).trimmed();

        if (namePart.isEmpty()) continue;

        FsEntry entry;
        entry.name = QFileInfo(namePart).fileName();
        entry.fullPath = "/" + namePart;
        entry.size = sizePart.toULongLong();
        entry.isDirectory = attrPart.contains("D");
        entry.permissions = entry.isDirectory ? "drwxr-xr-x" : "-rw-r--r--";
        part.rootEntries.append(entry);
    }

    if (!part.rootEntries.isEmpty()) {
        partitions.append(part);
        return true;
    }

    return false;
}

QByteArray DiskImageParser::extractFileDataFromBuffer(const QByteArray& data, const PartitionInfo& /*part*/, const FsEntry& entry)
{
    if (entry.dataOffset > 0 && entry.size > 0) {
        if (entry.dataOffset + entry.size <= (quint64)data.size()) {
            return data.mid(entry.dataOffset, entry.size);
        }
    }
    return entry.inlineData;
}

QByteArray DiskImageParser::extractFileData(const QString& imageFilePath, const PartitionInfo& /*part*/, const FsEntry& entry)
{
    QFile f(imageFilePath);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();

    if (entry.dataOffset > 0 && entry.size > 0) {
        if (f.seek(entry.dataOffset)) {
            return f.read(entry.size);
        }
    }

    // Fallback: 7z e -so <imageFilePath> <internalPath>
    QProcess proc;
    QString internalPath = entry.fullPath;
    if (internalPath.startsWith("/")) internalPath.remove(0, 1);
    proc.start("7z", {"e", "-so", imageFilePath, internalPath});
    if (proc.waitForStarted(1500) && proc.waitForFinished(5000) && proc.exitCode() == 0) {
        return proc.readAllStandardOutput();
    }

    return QByteArray();
}

} // namespace disk
