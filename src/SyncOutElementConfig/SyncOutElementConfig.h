#ifndef SyncOutElementConfig_h
#define SyncOutElementConfig_h

#include <ArduinoJson.h>
#include <StateMachine.h>

/*
 * Sync out options (defines how variable from state machine is emitted to the hub)
 *  {
 *    SYNC_TYPE: "i" | "p" | "c", // i - instant (on update), p - with preprocessing, c - on value change
 *    PREPROCESSING: "f" | "l" | "a" | "n" | "x",   // f - first, l - last, a - average, n - min, x - max
 *    FRAME_TYPE: "c" | "d", // c - number of state machine cycles, d - approximate duration in ms
 *    FRAME_LENGTH: <number>
 *    THRESHOLD: <number>
 *  }
 */

#define SYNC_TYPE "s"
#define SYNC_TYPE_INSTANT "i"
#define SYNC_TYPE_PREPROCESS "p"
#define SYNC_TYPE_ON_CHANGE "c"

#define SYNC_PREPROCESS "p"
#define SYNC_PREPROCESS_FIRST "f"
#define SYNC_PREPROCESS_LAST "l"
#define SYNC_PREPROCESS_AVERAGE "a"
#define SYNC_PREPROCESS_MIN "n"
#define SYNC_PREPROCESS_MAX "x"

#define SYNC_FRAME_TYPE "f"
#define SYNC_FRAME_TYPE_CYCLE_NUM "c"
#define SYNC_FRAME_TYPE_DURATION "d"

#define SYNC_FRAME_LENGTH "l"
#define SYNC_THRESHOLD "t"

#define T_INSTANT 1
#define T_PREPROCESS 2
#define T_ON_CHANGE 3

#define P_FIRST 1
#define P_LAST 2
#define P_AVERAGE 3
#define P_MIN 4
#define P_MAX 5

#define F_CYCLE_NUM 1
#define F_DURATION 2

class SyncOutElementConfig
{
public:
    SyncOutElementConfig(const char *, JsonVariant);

    uint8_t syncType = 0;
    uint8_t preprocessing = 0;
    uint8_t frameType = 0;
    unsigned long frameLength = 0;
    float threshold = 0.0f;

    const char *name;
    VarStruct accumulator;
    unsigned long updateCounter = 0;
    bool canEmit = false;
    unsigned long lastEmit = 0;
    unsigned long frameNum = 0;
};

#endif