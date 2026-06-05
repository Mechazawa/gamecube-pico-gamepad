#include <Arduino.h>
#include "pico/bootrom.h"
#include "Controller.h"
#include "Controller.pio.h"

// Gap left on the bus after each transaction before the next poll. The Joybus
// link itself runs at ~1 poll / 300us; the USB input endpoint is interrupt/1ms,
// so polling faster than 1kHz cannot make the host see fresher data. This gap
// keeps the controller comfortably within that budget while minimising the age
// of the sample sent on each USB frame.
static const unsigned kInterPollGapUs = 100;

Controller::Controller(InitParams *initParams, uint8_t sizeofControllerState) {
    _pin = initParams->pin;
    _pio = initParams->pio;
    _sm = initParams->sm;
    _c = initParams->c;
    _offset = initParams->offset;
    _sizeofControllerState = sizeofControllerState;
    _controllerState = new uint8_t[_sizeofControllerState];
}

void Controller::initPio(InitParams *initParams) {
    pio_hw_t *pios[2] = {pio0, pio1};
    uint pio_index = 0;
    if (!pio_can_add_program(pios[pio_index], &controller_program)) {
        pio_index = 1;
        if (!pio_can_add_program(pios[pio_index], &controller_program)) {
            // temp for development, reset the board if no PIO is available
            reset_usb_boot(0, 0);
        }
    }
    initParams->pio = pios[pio_index];
    initParams->offset = pio_add_program(initParams->pio, &controller_program);
    initParams->sm = pio_claim_unused_sm(initParams->pio, true);
    pio_sm_config tmpConfig =
            controller_program_get_default_config(initParams->offset);
    initParams->c = &tmpConfig;
    controller_program_init(initParams->pio, initParams->sm, initParams->offset,
                            initParams->pin, initParams->c);

    // Send a command to see if connected controller is N64 or Gamecube
    uint8_t initRequest[1] = {0x00};
    uint8_t initResponse[3] = {0x00};
    transfer(initParams->pio, initParams->sm, initParams->offset, initRequest,
             sizeof(initRequest), initResponse, sizeof(initResponse));
}

void Controller::transfer(uint8_t *request, uint8_t requestLength,
                          uint8_t *response, uint8_t responseLength) {
    transfer(_pio, _sm, _offset, request, requestLength, response, responseLength);
}

void Controller::transfer(PIO pio, uint sm, uint offset, uint8_t *request,
                          uint8_t requestLength, uint8_t *response,
                          uint8_t responseLength) {
    pio_sm_clear_fifos(pio, sm);
    pio_sm_put_blocking(pio, sm, ((responseLength - 1) & 0x1F) << 24);
    sendRequest(pio, sm, request, requestLength);
    if (!getResponse(pio, sm, response, responseLength)) {
        // A timeout leaves the SM wedged in the receive section (blocked on
        // `wait 0 pin 0`); reset it back to the send entry so the next poll
        // recovers instead of failing forever (e.g. after a hot-swap).
        pio_sm_set_enabled(pio, sm, false);
        pio_sm_restart(pio, sm);
        pio_sm_exec(pio, sm, pio_encode_jmp(offset));
        pio_sm_set_enabled(pio, sm, true);
    }
    delayMicroseconds(kInterPollGapUs);
}

void Controller::sendRequest(PIO pio, uint sm, uint8_t *request,
                             uint8_t requestLength) {
    int8_t remainingRequestBytes = requestLength;
    while (remainingRequestBytes > 0) {
        pio_sm_put_blocking(pio, sm,
                            request[requestLength - remainingRequestBytes] << 24);
        remainingRequestBytes--;
    }
}

bool Controller::getResponse(PIO pio, uint sm, uint8_t *response,
                             uint8_t responseLength) {
    int16_t remainingResponseBytes = responseLength;
    while (remainingResponseBytes > 0) {
        unsigned long timeout_us = micros() + 600;
        bool timedOut = false;
        while (pio_sm_is_rx_fifo_empty(pio, sm) && !timedOut) {
            timedOut = micros() > timeout_us;
        }
        if (timedOut) {
            return false; // Timeout occurred
        }
        uint32_t data = pio_sm_get(pio, sm);
        response[responseLength - remainingResponseBytes] = (uint8_t) (data & 0xFF);
        remainingResponseBytes--;
    }
    return true;
}

void Controller::init() {
    uint8_t request[1] = {0x41};
    uint8_t response[3];
    transfer(request, sizeof(request), response, sizeof(response));
}

void Controller::updateState() {
    uint8_t setRumble = _rumble ? 1 : 0;
    uint8_t request[3] = {0x40, 0x03, setRumble};
    transfer(request, sizeof(request), _controllerState, _sizeofControllerState);
}
