// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QSpacerItem>

#include "core/debugger/debug_interface.h"

#include "registers.h"

RegistersWidget::RegistersWidget(QWidget* parent) : QDockWidget(parent)
{
    cpu_regs_ui.setupUi(this);

    for (int i = 0; i < g_debug->GetNumRegsInCategory(REGCAT_CPSR_FLAGS); i++)
    {
        const char* name = g_debug->GetRegName(REGCAT_CPSR_FLAGS,i);

        QCheckBox* box = new QCheckBox(name,this);
        cpu_regs_ui.flags->addWidget(box);
        connect(box,SIGNAL(stateChanged(int)),this,SLOT(OnFlagToggled(int)));

        flagLookup[box] = i;
    }

    cpu_regs_ui.flags->addItem(new QSpacerItem(0,0,QSizePolicy::Minimum,QSizePolicy::Expanding));

    // reload cpsr flags when register values have been changed
    connect(cpu_regs_ui.registerView,SIGNAL(registerValueChanged()),this,SLOT(OnDebugModeEntered()));
}

void RegistersWidget::OnFlagToggled(int state)
{
    QCheckBox* sender = dynamic_cast<QCheckBox*>(this->sender());
    int index = flagLookup[sender];

    g_debug->SetRegValue(REGCAT_CPSR_FLAGS,index,state == 0 ? 0 : 1);
    cpu_regs_ui.registerView->update();
}

void RegistersWidget::OnDebugModeEntered()
{
    for (auto it = flagLookup.begin(); it != flagLookup.end(); it++)
    {
        QCheckBox* box = it.key();
        int index = it.value();

        bool check = g_debug->GetRegValue(REGCAT_CPSR_FLAGS,index) != 0;
        box->blockSignals(true);
        box->setChecked(check);
        box->blockSignals(false);
    }

    cpu_regs_ui.registerView->update();
}

void RegistersWidget::OnDebugModeLeft()
{

}
