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


#include "WatchUI.h"

#include "MainMenuTable.h"
#include "uif-ui-collection-wrd/WRDTableView.h"

#include "uif-ui-watch-face-wrd/WatchFaceUI.h"
#include "uif-ui-watch-face-wrd/SetTimeMenuTable.h"
#include "uif-ui-popup-alert-wrd/AlertView.h"

#include "message-center/MessageCenter.h"

#include "minar-platform/minar_platform.h"
#include "core-util/CriticalSectionLock.h"

#include "mbed-time/mbed-time.h"
#include "core-util/sbrk.h"

#if YOTTA_CFG_HARDWARE_WRD_SWO_PRESENT
#include "swo/swo.h"
#define printf(...) { swoprintf(__VA_ARGS__); }
#else
#define printf(...)
#endif

#if YOTTA_CFG_HARDWARE_WRD_TOUCH_PRESENT
#define SLIDER_1 YOTTA_CFG_HARDWARE_WRD_TOUCH_SLIDER_1
#define SLIDER_2 YOTTA_CFG_HARDWARE_WRD_TOUCH_SLIDER_2
#define SLIDER_3 YOTTA_CFG_HARDWARE_WRD_TOUCH_SLIDER_3
#define SLIDER_4 YOTTA_CFG_HARDWARE_WRD_TOUCH_SLIDER_4
#else
#define SLIDER_1 0
#define SLIDER_2 0
#define SLIDER_3 0
#define SLIDER_4 0
#endif

#if (YOTTA_CFG_HARDWARE_WRD_BUTTON_PRESENT) && \
    (YOTTA_CFG_HARDWARE_WRD_BUTTON_SIZE > 0)
#else
#error WRD Button not defined
#endif

#if (YOTTA_CFG_HARDWARE_WRD_TOUCH_PRESENT) && \
    defined(YOTTA_CFG_HARDWARE_WRD_TOUCH_SLIDER_BACK)
#define BUTTON_BACK YOTTA_CFG_HARDWARE_WRD_TOUCH_SLIDER_BACK
#elif defined(YOTTA_CFG_HARDWARE_WRD_BUTTON_BACKWARD_GPIO_PIN)
#define BUTTON_BACK YOTTA_CFG_HARDWARE_WRD_BUTTON_BACKWARD_GPIO_PIN
#else
#error WRD Back button not defined
#endif

WatchUI::WatchUI()
    :   forwardButton(YOTTA_CFG_HARDWARE_WRD_BUTTON_BUTTON0_PIN,
                      YOTTA_CFG_HARDWARE_WRD_BUTTON_BUTTON0_LOCATION),
        backButton(BUTTON_BACK),
        forwardCounter(0),
        backwardCounter(0),
        buttonLastPress(0),
        buttonHandle(NULL),
        goBackHandle(NULL),
        goWayBackHandle(NULL),
        resetHandle(NULL)
{
    /**************************************************************************
      Initialize slider.

      The slider is shared among all the views.
    **************************************************************************/

    const uint32_t sliderChannels[4] = {SLIDER_1, SLIDER_2, SLIDER_3, SLIDER_4};

    slider = SharedPointer<AnalogSlider>(new AnalogSlider(sliderChannels, 4));

    /*  Save energy by shutting down slider on watchface screen.
    */
    slider->pause();

    /**************************************************************************
      User buttons
    **************************************************************************/
#if defined(TARGET_LIKE_WATCH)
#elif defined(TARGET_LIKE_EFM32GG_STK)
    forwardButton.mode(PullUp);
    backButton.mode(PullUp);
    wait(.01);
#endif

    // button
    forwardButton.fall(this, &WatchUI::buttonPressISR);
    forwardButton.rise(this, &WatchUI::buttonReleaseISR);

    backButton.fall(this, &WatchUI::goBackPressISR);
    backButton.rise(this, &WatchUI::goBackReleaseISR);

    /**************************************************************************
      UIViewStack

      The stack holds all the previous views and handles animated transitions
      between them when objects are pushed/popped from the stack. The stack
      will prevent the first object pushed to it from ever being popped.
    **************************************************************************/
    stack = SharedPointer<UIViewStack>(new UIViewStack());
    stack->setWidth(128);
    stack->setHeight(128);
    stack->setTransitionTime(200);

    /*  The WatchFace is instantiated and pushed to the stack as the first
        object.
    */
    SharedPointer<UIView> view(new WatchFaceUI());
    stack->pushView(view);

    /*  The mainMenu object maintains state eventhough it has been popped from
        the stack, as oppposed to the other view objects that we do not hold
        pointers to after they have been popped. This allows the user to
        quickly see what time it is without loosing context on the main menu.
    */
    SharedPointer<UIView::Array> table(new MainMenuTable());
    mainMenu = SharedPointer<UIView>(new WRDTableView(table, slider));

    /*  The AlertView displays alerts on top of everything, across all applications.
    */
    alertCell = SharedPointer<UIView>(new AlertView());
}

WatchUI::~WatchUI()
{
    if (buttonHandle)
    {
        minar::Scheduler::cancelCallback(buttonHandle);
    }

    if (goBackHandle)
    {
        minar::Scheduler::cancelCallback(goBackHandle);
    }

    if (goWayBackHandle)
    {
        minar::Scheduler::cancelCallback(goWayBackHandle);
    }

    if (resetHandle)
    {
        minar::Scheduler::cancelCallback(resetHandle);
    }
}

/**************************************************************************
  Task for waking up the screen.
**************************************************************************/
void WatchUI::wakeupTask()
{
    if(wakeupCallback)
    {
        wakeupCallback.call();
    }
}

/**************************************************************************
  Task to handle backward button presses.

  This task suspends the current view, removes it from the stack, sets the
  next object in the stack as the current view, and sends it the resume
  command. The stack class will ensure that the last object cannot be
  popped.
**************************************************************************/
void WatchUI::goBackPressISR()
{
    goWayBackHandle = minar::Scheduler::postCallback(this, &WatchUI::goWayBackTask)
                        .delay(minar::milliseconds(300))
                        .tolerance(1)
                        .getHandle();
}

void WatchUI::goBackReleaseISR()
{
    if (goWayBackHandle != NULL)
    {
        minar::Scheduler::cancelCallback(goWayBackHandle);
        goWayBackHandle = NULL;

        backwardCounter++;

        if (goBackHandle == NULL)
        {
            goBackHandle = minar::Scheduler::postCallback(this, &WatchUI::goBackTask)
                            .tolerance(0)
                            .getHandle();
        }
    }
}

void WatchUI::goBackTask()
{
    stack->popView();

    /*  Disable the slider on the watchface screen to save power. */
    if (stack->getSize() == 1)
    {
        slider->pause();
    }
    else
    {
        slider->resume();
    }

    /* Signal UIKitFramework that the screen needs to be updated. */
    wakeupTask();

    /*  atomically decrement counter and repost current task
        if there are outstanding button presses
    */
    {
        mbed::util::CriticalSectionLock lock;

        backwardCounter--;

        if (backwardCounter > 0)
        {
            goBackHandle = minar::Scheduler::postCallback(this, &WatchUI::goBackTask)
                            .tolerance(0)
                            .getHandle();
        }
        else
        {
            goBackHandle = NULL;
        }
    }
}

void WatchUI::goWayBackTask()
{
    {
        mbed::util::CriticalSectionLock lock;
        goWayBackHandle = NULL;
    }

    printf("way back: %u\r\n", minar::platform::getTime());

    stack->resetView();

    /*  Disable the slider on the watchface screen to save power. */
    slider->pause();

    /* Signal UIKitFramework that the screen needs to be updated. */
    wakeupTask();
}

/**************************************************************************
  Task to handle forward button presses.

  If the WatchFace is currently showing, the mainMenu is swapped in and
  pushed to the stack. Otherwise the stack is send the action method.
  The return type can be:
    (UIView::Action::None)  : do nothing
    (UIView::Action::View)  : use object as view and push it to the stack
    (UIView::Action::Back)  : call goBackTask togo to previous screen.
**************************************************************************/
void WatchUI::buttonPressISR()
{
    // store time to check for long press
    buttonLastPress = minar::platform::getTime();

    // schedule new reset task
    resetHandle = minar::Scheduler::postCallback(this, &WatchUI::resetTask)
                    .delay(minar::milliseconds(5000))
                    .tolerance(1)
                    .getHandle();

    // normal press
    forwardCounter++;

    if (buttonHandle == NULL)
    {
        buttonHandle = minar::Scheduler::postCallback(this, &WatchUI::buttonTask)
                            .tolerance(0)
                            .getHandle();
    }
}

void WatchUI::buttonReleaseISR()
{
    uint32_t now = minar::platform::getTime();

    // cancel callback if button was released before timeout
    if (minar::ticks(now - buttonLastPress) < 5000)
    {
        minar::Scheduler::cancelCallback(resetHandle);
        resetHandle = NULL;
    }
}

void WatchUI::resetTask()
{
    mbed::time::saveOffset();

    NVIC_SystemReset();
    printf("software reset!\r\n");
}

void WatchUI::buttonTask()
{
    /*  Watchface is showing, swap in the main menu. */
    if (stack->getSize() == 1)
    {
        stack->pushView(mainMenu);
    }
    /* Send button command to the currently showing view if supported. */
    else
    {
        SharedPointer<UIView::Action> actionObject = stack->getAction();

        /* Received View. Push it to stack and show it. */
        if (actionObject->getType() == UIView::Action::View)
        {
            SharedPointer<UIView> view = actionObject->getView();
            stack->pushView(view);
        }
        /* Received control packet. Go back one screen. */
        else if (actionObject->getType() == UIView::Action::Back)
        {
            /*  atomically increment counter and post goBackTask
                if it has not already been posted.
            */
            {
                mbed::util::CriticalSectionLock lock;

                backwardCounter++;

                if (goBackHandle == NULL)
                {
                    goBackHandle = minar::Scheduler::postCallback(this, &WatchUI::goBackTask)
                                    .tolerance(0)
                                    .getHandle();
                }
            }
        }
    }

    /*  Disable the slider on the watchface screen to save power. */
    if (stack->getSize() == 1)
    {
        slider->pause();
    }
    else
    {
        slider->resume();
    }

    /* Signal UIKitFramework that the screen needs to be updated. */
    wakeupTask();

    /*  atomically decrement counter and repost current task
        if there are outstanding button presses
    */
    {
        mbed::util::CriticalSectionLock lock;

        forwardCounter--;

        if (forwardCounter > 0)
        {
            buttonHandle = minar::Scheduler::postCallback(this, &WatchUI::buttonTask)
                                .tolerance(0)
                                .getHandle();
        }
        else
        {
            buttonHandle = NULL;
        }
    }
}

/******************************************************************************
  UIView
******************************************************************************/

uint32_t WatchUI::fillFrameBuffer(SharedPointer<FrameBuffer>& canvas, int16_t xOffset, int16_t yOffset)
{
    uint32_t callInterval = ULONG_MAX;

    /* Route screen drawing throught the UIViewStack. */
    callInterval = stack->fillFrameBuffer(canvas, xOffset, yOffset);

    /* Show alert on screen */
    uint32_t alertInterval = alertCell->fillFrameBuffer(canvas, xOffset, yOffset);

    /* return minimum callback time */
    if (callInterval > alertInterval)
    {
        callInterval = alertInterval;
    }

    return callInterval;
}

void WatchUI::setWakeupCallback(FunctionPointer& callback)
{
    printf("WatchUI: set wakeup %p\r\n", callback.get_function());

    /* keep a reference for own use. */
    wakeupCallback = callback;

    /* propagate reference to child nodes. */
    stack->setWakeupCallback(callback);

    alertCell->setWakeupCallback(callback);
}
