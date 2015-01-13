// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QFontMetrics>

#include "util.h"

DebuggerFont::DebuggerFont() {
    font_ = QFont("Lucida Console", 10);
    font_.setStyleHint(QFont::Courier, QFont::PreferMatch);
    updateMetrics();
}

void DebuggerFont::setBold(bool bold) {
    font_.setBold(bold);
    updateMetrics();
}

QPoint DebuggerFont::calculateDrawPos(int left, int top) {
    return QPoint(left, top + charHeight() - charDescent());
}

void DebuggerFont::updateMetrics() {
    QFontMetrics metrics = QFontMetrics(font_);
    width = metrics.width('0');
    height = metrics.height();
    descent = metrics.descent();
}
