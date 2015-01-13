// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "debug_interface.h"

#include "core/core.h"
#include "core/arm/arm_interface.h"

DebugInterface g_debug_interface;
DebugInterface* g_debug = &g_debug_interface;

const char* reg_names_cat0[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",
    "r5",  "r6",  "r7",  "r8",  "r9",
    "r10", "r11", "r12", "r13", "r14",
    "r15", "cpsr"
};

const char* reg_names_cat1[] = {
    "m",   "it",  "ge",  "dnm",
};

const char* reg_names_cat2[] = {
    "n", "z", "c", "v", "q", "j",
    "e", "a", "i", "f", "t"
};

const u8 cat2_regs_cpsr_bit_indices[] = {
    31, 30, 29, 38, 27, 24,
    9,  8,  7,  6,  5
};

class ArmExpressionFunctions: public IExpressionFunctions {

public:
    ArmExpressionFunctions(DebugInterface* debug) {
        this->debug = debug;
    }

    virtual bool parseReference(char* str, uint32& reference_index)  override {
        // check if it's a register
        int index = 0;
        for (int i = 0; i < debug->GetNumCategories(); i++) {
            for (int l = 0; l < debug->GetNumRegsInCategory(i); l++) {
                if (strcasecmp(str,debug->GetRegName(i,l)) == 0) {
                    reference_index = index;
                    return true;
                }
                index++;
            }
        }

        return false;
    }

    virtual bool parseSymbol(char* str, uint32& value) {
        // todo: check labels
        return false;
    }

    virtual uint32 getReferenceValue(uint32 reference_index) {
        // find category the index belongs to and return the register value
        for (int i = 0; i < debug->GetNumCategories(); i++) {
            if (reference_index < debug->GetNumRegsInCategory(i))
                return debug->GetRegValue(i,reference_index);

            reference_index -= debug->GetNumRegsInCategory(i);
        }

        // should never be reached
        return 0;
    }

    virtual ExpressionType getReferenceType(uint32 referenceIndex) {
        return EXPR_TYPE_UINT;
    }

    virtual bool getMemoryValue(uint32 address, int size, uint32& dest, char* error) {
        // todo: implement
        return false;
    }

private:
    DebugInterface* debug;
};


bool DebugInterface::InitExpression(const char* exp, PostfixExpression& dest) {
    ArmExpressionFunctions funcs(this);
    return initPostfixExpression(exp,&funcs,dest);
}

bool DebugInterface::ParseExpression(PostfixExpression& exp, u32& dest) {
    ArmExpressionFunctions funcs(this);
    return parsePostfixExpression(exp,&funcs,dest);
}

int DebugInterface::GetNumCategories() {
    return 3;
}

int DebugInterface::GetNumRegsInCategory(int cat) {
    switch (cat) {
    case 0:
        return ARRAY_SIZE(reg_names_cat0);
    case 1:
        return ARRAY_SIZE(reg_names_cat1);
    case 2:
        return ARRAY_SIZE(reg_names_cat2);
    }

    return 0;
}

const char *DebugInterface::GetRegName(int cat, int index) {
    switch (cat) {
    case 0:
        return reg_names_cat0[index];
    case 1:
        return reg_names_cat1[index];
    case 2:
        return reg_names_cat2[index];
    }

    return nullptr;
}

u32 DebugInterface::GetRegValue(int cat, int index) {
    ARM_Interface* app = Core::g_app_core;
    if (app == nullptr)
        return 0;

    switch (cat) {
    case 0:
        if (index < 16)
            return app->GetReg(index);
        return app->GetCPSR();
    case 1:
        switch (index) {
        case CAT1_REG_M:
            return app->GetCPSR() & 0x1F;
        case CAT1_REG_IT:
            return (app->GetCPSR() >> 10) & 0x3F;
        case CAT1_REG_GE:
            return (app->GetCPSR() >> 16) & 0xF;
        case CAT1_REG_DNM:
            return (app->GetCPSR() >> 20) & 0xF;
        }
        return 0;
    case 2:
        return (app->GetCPSR() >> cat2_regs_cpsr_bit_indices[index]) & 1;
    }
}

void DebugInterface::SetRegValue(int cat, int index, u32 value) {
    ARM_Interface* app = Core::g_app_core;
    if (app == nullptr)
        return;

    u32 cpsr = app->GetCPSR();

    switch (cat) {
    case 0:
        if (index < 16)
            app->SetReg(index,value);
        else
            app->SetCPSR(value);
        break;
    case 1:
        switch (index) {
        case CAT1_REG_M:
            cpsr = (cpsr & ~0x1F) | (value & 0x1F);
            break;
        case CAT1_REG_IT:
            cpsr = (cpsr & ~(0x3F << 10)) | ((value & 0x3F) << 10);
            break;
        case CAT1_REG_GE:
            cpsr = (cpsr & ~(0xF << 16)) | ((value & 0xF) << 16);
            break;
        case CAT1_REG_DNM:
            cpsr = (cpsr & ~(0xF << 20)) | ((value & 0xF) << 20);
            break;
        }
        app->SetCPSR(cpsr);
        break;
    case 2:
        cpsr &= ~(1 << cat2_regs_cpsr_bit_indices[index]);
        if (value != 0)
            cpsr |= 1 << cat2_regs_cpsr_bit_indices[index];
        app->SetCPSR(cpsr);
        break;
    }
}
