#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);
    static void showAbout(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
};

#endif // ABOUTDIALOG_H
