#include "PidFfb.h"

#include <Arduino.h>
#include <string.h>

// Effect states
#define EFFECT_FREE      0x00
#define EFFECT_ALLOCATED 0x01
#define EFFECT_PLAYING   0x02

#define MAX_EFFECTS      14
#define SIZE_EFFECT      16
#define MEMORY_SIZE      ((uint16_t)(MAX_EFFECTS * SIZE_EFFECT))

// Effect operation values (PID "Effect Operation Array")
#define OP_START      1
#define OP_START_SOLO 2
#define OP_STOP       3

// Device control values
#define DC_ENABLE_ACTUATORS  1
#define DC_DISABLE_ACTUATORS 2
#define DC_STOP_ALL_EFFECTS  3
#define DC_RESET             4
#define DC_PAUSE             5
#define DC_CONTINUE          6

// ---- Wire layouts (packed; cast directly from the raw report buffer) ----
typedef struct __attribute__((packed)) {
    uint8_t reportId; // 1
    uint8_t effectBlockIndex;
    uint8_t effectType;
    uint16_t duration;
    uint16_t triggerRepeatInterval;
    uint16_t samplePeriod;
    uint8_t gain;
    uint8_t triggerButton;
    uint8_t enableAxis;
    uint8_t directionX;
    uint8_t directionY;
} SetEffect_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId; // 4
    uint8_t effectBlockIndex;
    uint16_t magnitude;
    int16_t offset;
    uint16_t phase;
    uint32_t period;
} SetPeriodic_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId; // 5
    uint8_t effectBlockIndex;
    int16_t magnitude;
} SetConstant_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId; // 6
    uint8_t effectBlockIndex;
    int16_t startMagnitude;
    int16_t endMagnitude;
} SetRamp_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId; // 10
    uint8_t effectBlockIndex;
    uint8_t operation;
    uint8_t loopCount;
} EffectOp_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId; // 11
    uint8_t effectBlockIndex;
} BlockFree_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId; // 12
    uint8_t control;
} DeviceControl_t;

// ---- GET response payloads (no report id; the host already knows it) ----
typedef struct __attribute__((packed)) {
    uint8_t effectBlockIndex;
    uint8_t loadStatus; // 1=success, 2=full, 3=error
    uint16_t ramPoolAvailable;
} BlockLoadResp_t;

typedef struct __attribute__((packed)) {
    uint16_t ramPoolSize;
    uint8_t maxSimultaneousEffects;
    uint8_t memoryManagement; // bit0 device-managed, bit1 shared param blocks
} PoolResp_t;

typedef struct __attribute__((packed)) {
    uint8_t status; // bit1 = actuators enabled
    uint8_t effectBlockIndex;
} StateResp_t;

namespace {

struct EffectState {
    uint8_t state;
    uint8_t effectType;
    int16_t magnitude;
    int16_t startMagnitude;
    int16_t endMagnitude;
};

EffectState g_effects[MAX_EFFECTS + 1];
uint8_t g_nextEID = 1;
uint16_t g_ramPoolAvailable = MEMORY_SIZE;

// Block-load result remembered between Create-New-Effect (set) and
// Block-Load (get).
uint8_t g_blIndex = 0;
uint8_t g_blStatus = 1;

int16_t g_lastMagnitude = 0;
uint32_t g_opCount = 0;

void freeAllEffects() {
    g_nextEID = 1;
    memset(g_effects, 0, sizeof(g_effects));
    g_ramPoolAvailable = MEMORY_SIZE;
}

uint8_t getNextFreeEffect() {
    if (g_nextEID == MAX_EFFECTS) {
        return 0;
    }
    uint8_t id = g_nextEID++;
    while (g_effects[g_nextEID].state != EFFECT_FREE) {
        if (g_nextEID >= MAX_EFFECTS) {
            break;
        }
        g_nextEID++;
    }
    g_effects[id].state = EFFECT_ALLOCATED;
    return id;
}

void startEffect(uint8_t id) {
    if (id >= 1 && id <= MAX_EFFECTS) {
        g_effects[id].state |= EFFECT_PLAYING;
    }
}

void stopEffect(uint8_t id) {
    if (id >= 1 && id <= MAX_EFFECTS) {
        g_effects[id].state &= ~EFFECT_PLAYING;
    }
}

void stopAllEffects() {
    for (uint8_t id = 1; id <= MAX_EFFECTS; id++) {
        g_effects[id].state &= ~EFFECT_PLAYING;
    }
}

void freeEffect(uint8_t id) {
    if (id == 0xFF) {
        freeAllEffects();
        return;
    }
    if (id >= 1 && id <= MAX_EFFECTS) {
        g_effects[id].state = EFFECT_FREE;
        if (id < g_nextEID) {
            g_nextEID = id;
        }
    }
}

int16_t effectiveMagnitude(const EffectState &e) {
    int16_t m = e.magnitude < 0 ? (int16_t)-e.magnitude : e.magnitude;
    int16_t s = e.startMagnitude < 0 ? (int16_t)-e.startMagnitude : e.startMagnitude;
    int16_t f = e.endMagnitude < 0 ? (int16_t)-e.endMagnitude : e.endMagnitude;
    if (s > m) {
        m = s;
    }
    if (f > m) {
        m = f;
    }
    return m;
}

} // namespace

namespace PidFfb {

void begin() {
    freeAllEffects();
}

void createNewEffect() {
    uint8_t id = getNextFreeEffect();
    if (id == 0) {
        g_blIndex = 0;
        g_blStatus = 2; // full
        return;
    }
    g_blIndex = id;
    g_blStatus = 1; // success
    memset(&g_effects[id], 0, sizeof(EffectState));
    g_effects[id].state = EFFECT_ALLOCATED;
    if (g_ramPoolAvailable >= SIZE_EFFECT) {
        g_ramPoolAvailable -= SIZE_EFFECT;
    }
}

void handleOutput(const uint8_t *report, uint16_t len) {
    if (len < 2) {
        return;
    }
    g_opCount++;
    uint8_t id = report[0];
    uint8_t idx = report[1]; // effectBlockIndex (valid for most reports)

    switch (id) {
        case 1: { // Set Effect
            const SetEffect_t *d = (const SetEffect_t *) report;
            if (idx >= 1 && idx <= MAX_EFFECTS) {
                g_effects[idx].effectType = d->effectType;
            }
            break;
        }
        case 4: { // Set Periodic
            const SetPeriodic_t *d = (const SetPeriodic_t *) report;
            if (idx >= 1 && idx <= MAX_EFFECTS) {
                g_effects[idx].magnitude = (int16_t) d->magnitude;
            }
            break;
        }
        case 5: { // Set Constant Force
            const SetConstant_t *d = (const SetConstant_t *) report;
            if (idx >= 1 && idx <= MAX_EFFECTS) {
                g_effects[idx].magnitude = d->magnitude;
            }
            break;
        }
        case 6: { // Set Ramp Force
            const SetRamp_t *d = (const SetRamp_t *) report;
            if (idx >= 1 && idx <= MAX_EFFECTS) {
                g_effects[idx].startMagnitude = d->startMagnitude;
                g_effects[idx].endMagnitude = d->endMagnitude;
            }
            break;
        }
        case 10: { // Effect Operation
            const EffectOp_t *d = (const EffectOp_t *) report;
            if (d->operation == OP_START) {
                startEffect(d->effectBlockIndex);
            } else if (d->operation == OP_START_SOLO) {
                stopAllEffects();
                startEffect(d->effectBlockIndex);
            } else if (d->operation == OP_STOP) {
                stopEffect(d->effectBlockIndex);
            }
            break;
        }
        case 11: { // Block Free
            const BlockFree_t *d = (const BlockFree_t *) report;
            freeEffect(d->effectBlockIndex);
            break;
        }
        case 12: { // Device Control
            const DeviceControl_t *d = (const DeviceControl_t *) report;
            if (d->control == DC_STOP_ALL_EFFECTS) {
                stopAllEffects();
            } else if (d->control == DC_RESET) {
                freeAllEffects();
            }
            break;
        }
        default:
            break; // envelope/condition/custom/gain: not needed for binary rumble
    }
}

uint16_t getBlockLoad(uint8_t *out) {
    BlockLoadResp_t r;
    r.effectBlockIndex = g_blIndex;
    r.loadStatus = g_blStatus;
    r.ramPoolAvailable = g_ramPoolAvailable;
    memcpy(out, &r, sizeof(r));
    return sizeof(r);
}

uint16_t getPool(uint8_t *out) {
    // The driver reads the pool report during init; treat it as a reset point.
    freeAllEffects();
    PoolResp_t r;
    r.ramPoolSize = MEMORY_SIZE;
    r.maxSimultaneousEffects = MAX_EFFECTS;
    r.memoryManagement = 3; // device managed + shared parameter blocks
    memcpy(out, &r, sizeof(r));
    return sizeof(r);
}

uint16_t getState(uint8_t *out) {
    StateResp_t r;
    r.status = 0x02; // actuators enabled
    r.effectBlockIndex = 0;
    memcpy(out, &r, sizeof(r));
    return sizeof(r);
}

bool rumbleRequested() {
    int16_t maxMag = 0;
    bool playing = false;
    for (uint8_t id = 1; id <= MAX_EFFECTS; id++) {
        if (g_effects[id].state & EFFECT_PLAYING) {
            int16_t m = effectiveMagnitude(g_effects[id]);
            if (m > maxMag) {
                maxMag = m;
            }
            if (m > 0) {
                playing = true;
            }
        }
    }
    g_lastMagnitude = maxMag;
    return playing;
}

int16_t lastMagnitude() {
    return g_lastMagnitude;
}

uint32_t opCount() {
    return g_opCount;
}

} // namespace PidFfb
