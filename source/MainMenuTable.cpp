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

#include "MainMenuTable.h"

#include "UIFramework/UIImageView.h"
#include "UIFramework/UITextView.h"
#include "UIFramework/UITextMonitorView.h"
#include "UIFramework/UITableView.h"

#include "SettingsMenuTable.h"

#include "uif-ui-battery-status-wrd/BatteryGaugeTable.h"
#include "uif-ui-touch-calibration-wrd/TouchCalibrationView.h"
#include "uif-ui-uptime-status-wrd/UptimeStatusTable.h"

#if 0
#include "swo/swo.h"
#define printf(...) { swoprintf(__VA_ARGS__); }
#else
#define printf(...)
#endif

typedef enum {
    CELL_TOP_FILLER = 0,
    CELL_BATTERY,
    CELL_TOUCH,
    CELL_STATS,
    CELL_SETTINGS,
    CELL_END_FILLER
} entries_t;

SharedPointer<UIView> MainMenuTable::viewAtIndex(uint32_t index) const
{
    UIView* cell = NULL;

    switch(index)
    {
        // top filler
        case CELL_TOP_FILLER:
                            cell = new UIImageView(NULL);
                            break;

        // get battery information
        case CELL_BATTERY:
                            cell = new UITextView("Battery", &Font_Menu);
                            break;

        // debug menu
        case CELL_TOUCH:
                            cell = new UITextView("Touch UI", &Font_Menu);
                            break;

        // stats menu
        case CELL_STATS:
                            cell = new UITextView("Statistics", &Font_Menu);
                            break;

        // settings menu
        case CELL_SETTINGS:
                            cell = new UITextView("Settings", &Font_Menu);
                            break;

        // bottom filler
        case CELL_END_FILLER:
        default:
                            cell = new UIImageView(NULL);
                            break;
    }

    if (cell)
    {
        cell->setHorizontalAlignment(UIView::ALIGN_LEFT);
        cell->setVerticalAlignment(UIView::VALIGN_MIDDLE);
        cell->setWidth(0);
        cell->setHeight(0);
    }

    return SharedPointer<UIView>(cell);
}

uint32_t MainMenuTable::getSize() const
{
    return CELL_END_FILLER + 1;
}

uint32_t MainMenuTable::widthAtIndex(uint32_t index) const
{
    (void) index;

    return 128;
}

uint32_t MainMenuTable::heightAtIndex(uint32_t index) const
{
    uint32_t ret = 0;

    if ( (index == CELL_TOP_FILLER) || (index == CELL_END_FILLER) )
    {
        ret = 128;
    }
    else
    {
        ret = 35;
    }

    return ret;
}

SharedPointer<UIView::Action> MainMenuTable::actionAtIndex(uint32_t index)
{
    SharedPointer<UIView::Action> returnObject;

    switch (index)
    {
        case CELL_BATTERY:
                            {
                                SharedPointer<UIView::Array> table(new BatteryGaugeTable());
                                SharedPointer<UIView> battery(new UITableView(table));

                                battery->setInverse(true);
                                battery->setWidth(128);
                                battery->setHeight(128);

                                returnObject = SharedPointer<UIView::Action>(new UIView::Action(battery));
                            }
                            break;

        case CELL_TOUCH:
                            {
                                SharedPointer<UIView> touch(new TouchCalibrationView());
                                returnObject = SharedPointer<UIView::Action>(new UIView::Action(touch));
                            }
                            break;

        case CELL_STATS:
                            {
                                SharedPointer<UIView::Array> table(new UptimeStatusTable());
                                SharedPointer<UIView> stats(new UITableView(table));

                                stats->setInverse(true);
                                stats->setWidth(128);
                                stats->setHeight(128);

                                returnObject = SharedPointer<UIView::Action>(new UIView::Action(stats));
                            }
                            break;

        case CELL_SETTINGS:
                            {
                                SharedPointer<UIView::Array> table(new SettingsMenuTable());
                                returnObject = SharedPointer<UIView::Action>(new UIView::Action(table));
                            }
                            break;
        default:
                            returnObject = SharedPointer<UIView::Action>(new UIView::Action);
                            break;
    }

    return returnObject;
}


const char* MainMenuTable::getTitle() const
{
    return "menu";
}

uint32_t MainMenuTable::getFirstIndex() const
{
    return CELL_TOP_FILLER + 1;
}

uint32_t MainMenuTable::getLastIndex() const
{
    return CELL_END_FILLER - 1;
}

uint32_t MainMenuTable::getDefaultIndex() const
{
    return CELL_BATTERY;
}
