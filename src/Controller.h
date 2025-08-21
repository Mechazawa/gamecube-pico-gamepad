#pragma once

#include <cstdio>
#include <Arduino.h>
#include "hardware/pio.h"

typedef struct {
    uint8_t pin;
    PIO pio;
    uint sm;
    pio_sm_config *c;
    uint offset;
} InitParams;

class Controller {
public:
    explicit Controller(InitParams *initParams, uint8_t sizeofControllerState = 8);

    void init();

    void setRumble(bool rumble) { _rumble = rumble; };

    static void initPio(InitParams *initParams);

    void transfer(uint8_t *request, uint8_t requestLength, uint8_t *response,
                  uint8_t responseLength);

    void updateState();

    uint8_t* getRawControllerState() const {
        return _controllerState;
    }

private:
    static void transfer(PIO pio, uint sm, uint8_t *request,
                         uint8_t requestLength, uint8_t *response,
                         uint8_t responseLength);

    static void sendRequest(PIO pio, uint sm, uint8_t *request,
                     uint8_t requestLength);

    static void getResponse(PIO pio, uint sm, uint8_t *response,
                     uint8_t responseLength);

protected:

    uint8_t _pin;
    PIO _pio;
    uint _sm;
    pio_sm_config *_c;
    uint _offset;
    uint8_t *_controllerState;
    uint8_t _sizeofControllerState;
    bool _rumble = false;
};
