// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstring>
#include <string>

#include "common/expression_parser.h"

enum RegisterCategory { REGCAT_GPR = 0, REGCAT_CPSR_FIELDS, REGCAT_CPSR_FLAGS };
enum Cat1Registers { CAT1_REG_M, CAT1_REG_IT, CAT1_REG_GE, CAT1_REG_DNM };

class DebugInterface {
public:
    ~DebugInterface() {}

    bool InitExpression(const char* exp, PostfixExpression& dest);
    bool ParseExpression(PostfixExpression& exp, u32& dest);

    u32 GetPC() { return GetRegValue(REGCAT_GPR,15); }
    int GetNumCategories();
    int GetNumRegsInCategory(int cat);
    const char *GetRegName(int cat, int index);
    u32 GetRegValue(int cat, int index);
    void SetRegValue(int cat, int index, u32 value);
};

extern DebugInterface* g_debug;
