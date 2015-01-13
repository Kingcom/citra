// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "ui_registers.h"

#include <QDockWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include <QCheckBox>

class QTreeWidget;


class RegistersWidget : public QDockWidget
{
    Q_OBJECT

public:
    RegistersWidget(QWidget* parent = NULL);

public slots:
    void OnDebugModeEntered();
    void OnDebugModeLeft();
    void OnFlagToggled(int state);

private:
    QMap<QCheckBox*,int> flagLookup;
    Ui::ARMRegisters cpu_regs_ui;
};
