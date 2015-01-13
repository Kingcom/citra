// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QPainter>
#include <QMouseEvent>
#include <QShortcut>

#include "core/debugger/debug_interface.h"

#include "register_view.h"

RegisterView::RegisterView(QWidget* parent) : QFrame(parent) {
    // initialize registers
    for (int cat = 0; cat < 2; cat++) {
        for (int index = 0; index < g_debug->GetNumRegsInCategory(cat); index++) {
            Register reg;
            reg.category = cat;
            reg.index = index;
            reg.oldValue = 0;
            reg.changed = false;

            registers.append(reg);
        }
    }
    
    initializeSize();

    last_pc = 0;
    selection = 0;

    QShortcut* up_shortcut = new QShortcut(QKeySequence(Qt::Key_Up), this);
    up_shortcut->setContext(Qt::WidgetShortcut);
    connect(up_shortcut, SIGNAL(activated()), this, SLOT(previousRegister()));

    QShortcut* down_shortcut = new QShortcut(QKeySequence(Qt::Key_Down), this);
    down_shortcut->setContext(Qt::WidgetShortcut);
    connect(down_shortcut, SIGNAL(activated()), this, SLOT(nextRegister()));

    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
}

void RegisterView::initializeSize() {
    // calculate max register name length
    int max_length = 0;
    for (int i = 0; i < registers.size(); i++) {
        const char* name = g_debug->GetRegName(registers[i].category,registers[i].index);
        max_length = qMax<int>(max_length, strlen(name));
    }

    // initialize positions
    border_gap = 3;
    value_offset = border_gap + (max_length + 1) * font.charWidth();
    
    // characters per line = length of register name + space + length of register value
    int numChars = max_length + 1 + 8;

    // calculate size of widget. borderGap pixels to all sides,
    // and as many rows and columns as needed
    QSize size = QSize(2 * border_gap + numChars*font.charWidth(),
        2 * border_gap + registers.size()*font.charHeight());

    // make sure the widget has exactly that size
    setMinimumSize(size);
    setMaximumSize(size);
}

void RegisterView::refreshChangedRegs() {
    // the registers need to be refreshed if the PC of the core has
    // changed since the last call
    if (g_debug->GetPC() == last_pc)
        return;

    for (Register& reg: registers) {
        unsigned int value = g_debug->GetRegValue(reg.category,reg.index);
        reg.changed = (value != reg.oldValue);
        reg.oldValue = value;
    }

    last_pc = g_debug->GetPC();
}

void RegisterView::paintEvent(QPaintEvent* event) {
    QPainter p(this);

    refreshChangedRegs();

    // clear background
    p.setBrush(QBrush(QColor(Qt::white)));
    p.setPen(QPen(QColor(Qt::white)));
    p.drawRect(0, 0, width(), height());
    
    p.setFont(font.font());
    
    QPoint pos;
    QString value;
    for (int i = 0; i < registers.size(); i++) {
        Register& reg = registers[i];
        int y = border_gap + i * font.charHeight();

        // highlight background of selected register
        if (i == selection)
        {
            QColor col(0xE8, 0xEF, 0xFF);
            p.setBrush(col);
            p.setPen(col);
            p.drawRect(0, y - 1, width(), font.charHeight() - 1);
        }

        // draw register name
        pos = font.calculateDrawPos(border_gap, y);
        p.setPen(QColor(0, 0, 0x60));
        p.drawText(pos, g_debug->GetRegName(reg.category, reg.index));

        // draw register value
        pos = font.calculateDrawPos(value_offset, y);
        value.sprintf("%08X", g_debug->GetRegValue(reg.category, reg.index));
        
        // draw changed registers in red, rest in black
        if (reg.changed)
            p.setPen(QColor(Qt::red));
        else
            p.setPen(QColor(Qt::black));

        p.drawText(pos, value);
    }

    // draw frame last, otherwise it gets overwritten
    QFrame::paintEvent(event);
}

int RegisterView::mapPositionToRegister(QPoint pos) const {
    int y = pos.y();

    if (y < border_gap || y + border_gap >= height())
        return -1;

    return (y - border_gap) / font.charHeight();
}

void RegisterView::mousePressEvent(QMouseEvent* event) {
    int new_selection = mapPositionToRegister(event->pos());
    if (new_selection != -1 && selection != new_selection) {
        selection = new_selection;
        update();
    }
}

void RegisterView::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & (Qt::MouseButton::LeftButton | Qt::MouseButton::RightButton)) {
        int new_selection = mapPositionToRegister(event->pos());
        if (new_selection != -1 && selection != new_selection)
        {
            selection = new_selection;
            update();
        }
    }
}

void RegisterView::moveSelectionUp() {
    selection = qMax<int>(0, selection - 1);
    update();
}

void RegisterView::moveSelectionDown() {
    selection = qMin<int>(registers.size() - 1, selection + 1);
    update();
}
