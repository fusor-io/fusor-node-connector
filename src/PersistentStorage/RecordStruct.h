#ifndef recordstruct_h
#define recordstruct_h

#include <Arduino.h>
#include <StateMachine.h>

typedef struct RecordStruct
{
    RecordStruct(VarStruct *value)
        : var(value), updatedAt(millis()) {}

    VarStruct var;
    unsigned long updatedAt;
} RecordStruct;

#endif