// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
#pragma once

#include <QFont>
#include <QPoint>

#include "common/common_types.h"

// Abstracts font calculations used by some
// debugger widgets
class DebuggerFont {
public:
    DebuggerFont();

    // Returns the position needed to draw a string that should
    // have its upper left corner at the given coordinates
    QPoint calculateDrawPos(int left, int top);

    QFont& font() { return font_; }
    int charWidth() const { return width; };
    int charHeight() const { return height; };
    int charDescent() const { return descent; };
    
    void setBold(bool bold);

private:
    void updateMetrics();

    QFont font_;
    int width;
    int height;
    int descent;
};

bool parseExpression(QString& exp, u32& dest);
