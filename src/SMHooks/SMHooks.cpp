#include <Arduino.h>

#include "SMHooks.h"

void SMHooks::onVarUpdate(const char *name, VarStruct *value)
{
    Serial.print("Updated ");
    Serial.print(name);
    Serial.print(" = ");
    Serial.println(value->vInt);
}