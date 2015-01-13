// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QFontMetrics>

#include "core/debugger/debug_interface.h"

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


bool parseExpression(QString& exp, u32& dest) {
	PostfixExpression postfix;
	if (!g_debug->InitExpression(exp.toLatin1().data(),postfix))
		return false;

	return g_debug->ParseExpression(postfix,dest);
}
