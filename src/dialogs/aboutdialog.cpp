#include "aboutdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QPixmap>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("О программе Amethyst"));
    setFixedSize(500, 360);
    setWindowIcon(QIcon(":/icons/icon.svg"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 20);
    mainLayout->setSpacing(16);

    // Header: icon + name & fork note
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    auto *iconLabel = new QLabel(this);
    QPixmap iconPixmap = QIcon(":/icons/icon.svg").pixmap(64, 64);
    if (iconPixmap.isNull()) {
        iconPixmap = QIcon(":/icons/amethyst.ico").pixmap(64, 64);
    }
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(64, 64);
    headerLayout->addWidget(iconLabel);

    auto *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    auto *titleLabel = new QLabel("<h2 style='margin: 0; padding: 0;'>Amethyst</h2>", this);
    auto *forkLabel = new QLabel(tr("<b>Форк среды разработки Cremniy</b>"), this);
    forkLabel->setStyleSheet("color: #b16cee; font-size: 13px; font-weight: bold;");
    auto *versionLabel = new QLabel(tr("Версия: 0.3.1 (Windows Edition)"), this);

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(forkLabel);
    titleLayout->addWidget(versionLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    mainLayout->addLayout(headerLayout);

    // Description text
    auto *descLabel = new QLabel(
        tr("<p><b>Amethyst</b> — среда для низкоуровневой разработки с фокусом на платформу Windows. "
           "Объединяет редактор кода, HEX-редактор и дизассемблер в едином рабочем пространстве.</p>"
           "<p>Является форком проекта <a href='https://github.com/munirov/cremniy'>Cremniy</a>, "
           "созданного Munirov и сообществом контрибьюторов.</p>"
           "<p><b>Репозиторий форка:</b> "
           "<a href='https://github.com/Atimenka/Amethyst'>https://github.com/Atimenka/Amethyst</a><br>"
           "<b>Оригинальный проект:</b> "
           "<a href='https://github.com/munirov/cremniy'>https://github.com/munirov/cremniy</a></p>"
           "<p style='color: #888888;'>Лицензия: GNU General Public License v3.0</p>"),
        this);
    descLabel->setWordWrap(true);
    descLabel->setOpenExternalLinks(true);
    mainLayout->addWidget(descLabel);

    mainLayout->addStretch();

    // Close button
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto *closeBtn = new QPushButton(tr("Закрыть"), this);
    closeBtn->setDefault(true);
    closeBtn->setMinimumWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void AboutDialog::showAbout(QWidget *parent) {
    AboutDialog dlg(parent);
    dlg.exec();
}
