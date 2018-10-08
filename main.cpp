/* SDT-example-ble-uart-echo
 * 
 * Copyright (c) 2018 Sigma Delta Technologies Inc.
 * 
 * MIT License
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "mbed.h"
#include "features/FEATURE_BLE/ble/BLE.h"
#include "features/FEATURE_BLE/ble/services/UARTService.h"

/* Serial */
#define BAUDRATE 9600
Serial g_Serial_pc(USBTX, USBRX, BAUDRATE);

/* DigitalOut */
#define LED_ON      0
#define LED_OFF     1
DigitalOut g_DO_LedRed(LED_RED, LED_OFF);       // For SDT52832B, the GPIO0 operates as an NFC function and cannot serve as LED0
DigitalOut g_DO_LedGreen(LED_GREEN, LED_OFF);   // For SDT52832B, the GPIO1 operates as an NFC function and cannot serve as LED1
DigitalOut g_DO_LedBlue(LED_BLUE, LED_OFF);
DigitalOut* g_pDO_Led = &g_DO_LedBlue;

/* Ticker */
Ticker g_Ticker;

/* BLE */
#define BLE_DEVICE_NAME "SDT Device"
BLE& g_pBle = BLE::Instance();      // you can't use this name, 'ble', because this name is already declared in UARTService.h

/* UART service */
UARTService* g_pUartService;

/* Variable */
bool g_b_BleConnect = false;



void callbackTicker(void) {
    *g_pDO_Led = !(*g_pDO_Led);
}

void callbackBleDataWritten(const GattWriteCallbackParams* params) {
    if ((g_pUartService != NULL) && (params->handle == g_pUartService->getTXCharacteristicHandle())) {
        uint16_t bytesRead = params->len;
        const uint8_t* pBleRxBuf = params->data;
        g_Serial_pc.printf("data from BLE: %s\r\n", pBleRxBuf);
        g_pBle.gattServer().write(g_pUartService->getRXCharacteristicHandle(), pBleRxBuf, bytesRead);
    }
}

void callbackBleConnection(const Gap::ConnectionCallbackParams_t* params) {
    g_Serial_pc.printf("Connected!\n");
    g_b_BleConnect = true;
    g_Ticker.attach(callbackTicker, 1);
}

void callbackBleDisconnection(const Gap::DisconnectionCallbackParams_t* params) {
    g_Serial_pc.printf("Disconnected!\n");
    g_Serial_pc.printf("Restarting the advertising process\n\r");
    g_b_BleConnect = false;
    g_Ticker.detach();
    *g_pDO_Led = LED_ON;
    g_pBle.gap().startAdvertising();
}

void callbackBleInitComplete(BLE::InitializationCompleteCallbackContext* params) {
    BLE& ble = params->ble;                     // 'ble' equals g_pBle declared in global
    ble_error_t error = params->error;          // 'error' has BLE_ERROR_NONE if the initialization procedure started successfully.

    if (error == BLE_ERROR_NONE) {
        g_Serial_pc.printf("Initialization completed successfully\n");
    }
    else {
        /* In case of error, forward the error handling to onBleInitError */
        g_Serial_pc.printf("Initialization failled\n");
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        g_Serial_pc.printf("ID of BLE instance is not DEFAULT_INSTANCE\n");
        return;
    }

    /* Setup UARTService */
    g_pUartService = new UARTService(ble);

    /* Setup and start advertising */
    ble.gattServer().onDataWritten(callbackBleDataWritten);
    ble.gap().onConnection(callbackBleConnection);
    ble.gap().onDisconnection(callbackBleDisconnection);
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000);     // Advertising interval in units of milliseconds
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME, (const uint8_t *)BLE_DEVICE_NAME, sizeof(BLE_DEVICE_NAME) - 1);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS, (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));
    ble.gap().startAdvertising();
    g_Serial_pc.printf("Start advertising\n");
}

int main(void) {
    g_Serial_pc.printf("< Sigma Delta Technologies Inc. >\n\r");

    /* Init BLE */
    // g_pBle.onEventsToProcess(callbackEventsToProcess);
    g_pBle.init(callbackBleInitComplete);

    /* Check whether IC is running or not */
    *g_pDO_Led = LED_ON;

    while (true) {
        g_pBle.waitForEvent();
    }

    return 0;
}
