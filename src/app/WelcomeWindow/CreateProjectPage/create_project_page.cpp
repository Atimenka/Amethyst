/*
 * This file is part of the Amethyst IDE source code.
 *
 * Copyright (c) 2026 Amethyst IDE / Cremniy IDE
 * SPDX-License-Identifier: GPL-3.0 license
 *
 * Repository:
 * https://github.com/Atimenka/Amethyst
 */

#include "create_project_page.h"
#include "widgets/clickablelineedit.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>

CreateProjectPage::CreateProjectPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("CreateProjectPage");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 30, 40, 30);
    root->setSpacing(16);

    /* Header */
    auto* header = new QHBoxLayout();
    header->setSpacing(12);

    auto* logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/icons/icon.svg");
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setFixedSize(32, 32);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    auto* title = new QLabel(tr("Новый проект Amethyst"), this);
    title->setObjectName("CreateProjectTitle");
    auto* subtitle = new QLabel(tr("Настройте параметры нового проекта для низкоуровневой разработки"), this);
    subtitle->setObjectName("CreateProjectSubtitle");
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);

    header->addWidget(logoLabel);
    header->addLayout(titleLayout);
    header->addStretch();

    root->addLayout(header);
    root->addSpacing(16);

    /* Form Grid */
    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);
    grid->setColumnStretch(1, 1);

    m_nameLabel = new QLabel(tr("Имя проекта:"), this);
    m_nameLabel->setObjectName("FormLabel");
    m_nameEdit  = new QLineEdit(this);
    m_nameEdit->setObjectName("FormInput");
    m_nameEdit->setPlaceholderText("my-project");
    m_nameEdit->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z0-9_-]+$"), this)
    );
    grid->addWidget(m_nameLabel, 0, 0);
    grid->addWidget(m_nameEdit,  0, 1);

    m_langLabel = new QLabel(tr("Основной язык:"), this);
    m_langLabel->setObjectName("FormLabel");
    m_langCombo = new QComboBox(this);
    m_langCombo->setObjectName("FormCombo");
    m_langCombo->addItems({"C", "C++", "ASM", "C + ASM", "Rust", "Custom"});
    grid->addWidget(m_langLabel, 1, 0);
    grid->addWidget(m_langCombo, 1, 1);

    m_pathLabel = new QLabel(tr("Расположение:"), this);
    m_pathLabel->setObjectName("FormLabel");
    m_pathEdit  = new ClickableLineEdit(this);
    m_pathEdit->setObjectName("FormInput");
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText(tr("Нажмите для выбора папки..."));
    m_pathEdit->setCursor(Qt::PointingHandCursor);
    connect(m_pathEdit, &ClickableLineEdit::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Выберите папку проекта"), QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
        if (!dir.isEmpty()) m_pathEdit->setText(dir);
    });
    grid->addWidget(m_pathLabel, 2, 0);
    grid->addWidget(m_pathEdit,  2, 1);

    root->addLayout(grid);
    root->addStretch();

    /* Error label */
    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName("CreateProjectInfoLabel");
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setVisible(false);
    root->addWidget(m_infoLabel);
    root->addSpacing(8);

    /* Buttons */
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto* backBtn   = new QPushButton(tr("← Назад"), this);
    backBtn->setObjectName("CreateProjectBackBtn");
    backBtn->setCursor(Qt::PointingHandCursor);

    auto* createBtn = new QPushButton(tr("Создать проект"), this);
    createBtn->setObjectName("CreateProjectSubmitBtn");
    createBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addStretch();
    btnLayout->addWidget(backBtn);
    btnLayout->addWidget(createBtn);
    root->addLayout(btnLayout);

    connect(backBtn,   &QPushButton::clicked, this, &CreateProjectPage::backRequested);
    connect(createBtn, &QPushButton::clicked, this, &CreateProjectPage::onCreateClicked);
}

void CreateProjectPage::onCreateClicked()
{
    resetErrors();

    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        setFieldError(m_nameLabel, tr("Пожалуйста, введите имя проекта"));
        return;
    }

    if (const QFileInfo dirInfo(m_pathEdit->text()); !dirInfo.exists() || !dirInfo.isDir()) {
        setFieldError(m_pathLabel, tr("Пожалуйста, выберите существующую папку"));
        return;
    }

    const QString newPath = m_pathEdit->text() + "/" + name;
    QDir dir;

    if (dir.exists(newPath)) {
        setFieldError(m_nameLabel, tr("Папка с таким именем уже существует"));
        return;
    }

    if (!dir.mkdir(newPath)) {
        m_infoLabel->setText(tr("Не удалось создать папку проекта"));
        m_infoLabel->setVisible(true);
        return;
    }

    emit projectCreated(newPath, m_langCombo->currentText());
}

void CreateProjectPage::setFieldError(QLabel* label, const QString& message) const
{
    label->setProperty("error", true);
    label->style()->polish(label);
    m_infoLabel->setText(message);
    m_infoLabel->setVisible(true);
}

void CreateProjectPage::resetErrors()
{
    m_infoLabel->setVisible(false);
    for (auto* lbl : {m_nameLabel, m_langLabel, m_pathLabel}) {
        lbl->setProperty("error", false);
        lbl->style()->polish(lbl);
    }
}
