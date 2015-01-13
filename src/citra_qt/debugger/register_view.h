// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
#pragma once

#include <QFrame>
#include "util.h"

class RegisterView: public QFrame {
    Q_OBJECT

public:
    RegisterView(QWidget* parent = NULL);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    // Select previous register
    void moveSelectionUp();

    // Select next register 
    void moveSelectionDown();

    // Copies a string representation of the selected
    // register's value to the clipboard
    void copyRegisterValue();

    void editRegisterValue();

signals:
    // Emitted when the value of a register was changed by the user
    void registerValueChanged();

private:
    // Reloads register values if necessary
    void refreshChangedRegs();

    // Calculates and sets the widget size and the used positions
    void initializeSize();

    // Returns the number of the register at the given position
    int mapPositionToRegister(QPoint pos) const;

    struct Register
    {
        unsigned int oldValue;
        bool changed;
        int category;
        int index;
    };

    DebuggerFont font;
    int border_gap;
    int value_offset;

    unsigned int last_pc;
    QVector<Register> registers;
    int selection;
};
