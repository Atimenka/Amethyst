#include "helpmenu.h"
#include "ui/MenuBar/menufactory.h"
#include "dialogs/aboutdialog.h"

static bool registered = [](){
    MenuFactory::instance().registerMenu("8", [](){
        return new HelpMenu();
    });
    return true;
}();

HelpMenu::HelpMenu() : BaseMenu(tr("Справка")) {
    m_aboutAction = new QAction(tr("О программе Amethyst..."), this);
    this->addAction(m_aboutAction);
}

void HelpMenu::setupConnections(IDEWindow* ideWind) {
    connect(m_aboutAction, &QAction::triggered, ideWind, [ideWind]() {
        AboutDialog::showAbout(ideWind);
    });
}
