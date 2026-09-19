#ifndef HELPMENU_H
#define HELPMENU_H

#include "ui/MenuBar/basemenu.h"

class HelpMenu : public BaseMenu
{
    Q_OBJECT
private:
    QAction* m_aboutAction;
public:
    HelpMenu();
    void setupConnections(IDEWindow* ideWind) override;
};

#endif // HELPMENU_H
