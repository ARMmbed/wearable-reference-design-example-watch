/* mbed
 * Copyright (c) 2006-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed-drivers/mbed.h"

#include "wrd-utilities/SharedModules.h"

#include "uif-matrixlcd/MatrixLCD.h"
#include "UIFramework/UIFramework.h"
#include "WatchUI.h"

#include "message-center/MessageCenter.h"
#include "message-center-transport/MessageCenterSPIMaster.h"
#include "cborg/Cbor.h"
#include "mbed-block/BlockDynamic.h"

#include "wrd-gpio-switch/DigitalOutEx.h"

#include <string>

#if YOTTA_CFG_HARDWARE_WRD_WATCHDOG_PRESENT
#include "em_wdog.h" // from mbed-hal-silabs
#endif

#if (YOTTA_CFG_HARDWARE_WRD_SWO_PRESENT \
  && YOTTA_CFG_HARDWARE_WRD_SWO_ENABLED)
#include "swo/swo.h"
#define DEBUGOUT(...) { swoprintf(__VA_ARGS__); }
#else
#define DEBUGOUT(...)
#endif

#if YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_PRESENT
static DigitalOutEx blePower(YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_ENABLE_PIN,
                             1,
                             YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_ENABLE_LOCATION);

#define BLE_NAME     YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_SPI_NAME
#define BLE_NRDY     YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_NRDY_PIN
#define BLE_CS       YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_NREQ_PIN

// message center
static MessageCenterSPIMaster* transport;
#endif

#if YOTTA_CFG_HARDWARE_WRD_VIBRATION_MOTOR_PRESENT
#include "wrd-vibration-motor/VibrationMotor.h"
static VibrationMotor vibra;
#endif

#if YOTTA_CFG_HARDWARE_WRD_RGB_LED_PRESENT
#include "wrd-rgb-led/RGBLED.h"
static RGBLED led;
#endif

static uif::MatrixLCD lcd;
static SharedPointer<UIFramework> uiFramework;

/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/

bool radioOn = true;
bool connected = false;

std::string deviceName = "";
SharedPointer<BlockStatic> sendBlock;

uint32_t alertTimeOnScreen = 5000;

/*****************************************************************************/

void sendTaskDone()
{
    // free block
    sendBlock = SharedPointer<BlockStatic>();
}

void receivedControl(BlockStatic block)
{
    DEBUGOUT("main:control: ");
    for (uint32_t idx = 0; idx < block.getLength(); idx++)
    {
        DEBUGOUT("%02X", block.at(idx));
    }
    DEBUGOUT("\r\n");

    Cborg decoder(block.getData(), block.getLength());

    uint32_t type = 0;
    uint32_t value = 0;

    decoder.at(0).getUnsigned(&type);
    decoder.at(1).getUnsigned(&value);

    if (type == 1)
    {
        if (value == 1)
        {
            connected = true;
#if 1
            sendBlock = SharedPointer<BlockStatic>(new BlockDynamic(1 + 4 + 4 + 9 + 9));
            Cbore encoder(sendBlock->getData(), sendBlock->getLength());

            encoder.array(3)
                   .item(alertTimeOnScreen)
                   .item("Bluetooth")
                   .item("Connected");

            sendBlock->setLength(encoder.getLength());

            MessageCenter::sendTask(MessageCenter::LocalHost,
                                    MessageCenter::AlertPort,
                                    *sendBlock.get(),
                                    sendTaskDone);
#endif
        }
        else if (value == 2)
        {
            connected = false;

#if 1
            sendBlock = SharedPointer<BlockStatic>(new BlockDynamic(1 + 4 + 4 + 9 + 12));
            Cbore encoder(sendBlock->getData(), sendBlock->getLength());

            encoder.array(3)
                   .item(alertTimeOnScreen)
                   .item("Bluetooth")
                   .item("Disconnected");

            sendBlock->setLength(encoder.getLength());

            MessageCenter::sendTask(MessageCenter::LocalHost,
                                    MessageCenter::AlertPort,
                                    *sendBlock.get(),
                                    sendTaskDone);
#endif
        }
    }
}

/*****************************************************************************/

#if YOTTA_CFG_HARDWARE_WRD_WATCHDOG_PRESENT
static void watchdogFeeder()
{
    WDOG_Feed();
}
#endif

static void addTransport()
{
    transport = new MessageCenterSPIMaster(BLE_NAME, BLE_CS, BLE_NRDY);
    MessageCenter::addTransportTask(MessageCenter::RemoteHost, transport);
}

void app_start(int, char *[])
{
#if YOTTA_CFG_HARDWARE_WRD_WATCHDOG_ENABLED
    // setup watch dog
    CMU_ClockEnable(cmuClock_CORELE, true);

    WDOG_Init_TypeDef config = WDOG_INIT_DEFAULT;
    config.enable   = true;
    config.debugRun = false;
    config.em2Run   = true;
    config.lock     = true;
    config.clkSel   = wdogClkSelLFXO;
    config.perSel   = wdogPeriod_256k;

    WDOG_Init(&config);

    minar::Scheduler::postCallback(watchdogFeeder)
        .period(minar::milliseconds(3500));
#endif

    // on and off chip communication
#if YOTTA_CFG_HARDWARE_WRD_BLUETOOTH_LE_PRESENT
    minar::Scheduler::postCallback(addTransport)
        .delay(minar::milliseconds(1000));
#endif

    MessageCenter::addListenerTask(MessageCenter::LocalHost,
                                   MessageCenter::ControlPort,
                                   receivedControl);

    // Watch application
    SharedPointer<UIView> view(new WatchUI());
    view->setWidth(128);
    view->setHeight(128);

    // UI framework
    uiFramework = SharedPointer<UIFramework>(new UIFramework(lcd, view));

    DEBUGOUT("watch demo\r\n");

#if YOTTA_CFG_HARDWARE_WRD_VIBRATION_MOTOR_PRESENT
    vibra.dutyCycle(70).on(100);
#endif

#if YOTTA_CFG_HARDWARE_WRD_RGB_LED_PRESENT
    led.set(0, 255, 0, 100);
#endif
}
