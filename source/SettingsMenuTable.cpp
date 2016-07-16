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


#include "SettingsMenuTable.h"

#include "UIFramework/UITextView.h"
#include "UIFramework/UITextMonitorView.h"

#include "uif-ui-watch-face-wrd/SetTimeMenuTable.h"

#include "message-center/MessageCenter.h"
#include "cborg/Cbor.h"

using namespace mbed::util;

typedef enum {
    CELL_TOP_FILLER       = 0, // Always first
    CELL_SET_TIME,
    CELL_RADIO_ON,
    CELL_RADIO_CONNECTED,
    // Always last
    CELL_END_FILLER
} entries_t;


SharedPointer<UIView> SettingsMenuTable::viewAtIndex(uint32_t index) const
{
    SharedPointer<UIView> cell;

    switch(index)
    {
        // top filler
        case CELL_TOP_FILLER:
                            cell = SharedPointer<UIView>(new UIImageView(NULL));
                            break;

        // set time and date
        case CELL_SET_TIME:
                            cell = SharedPointer<UIView>(new UITextView("Set time", &Font_Menu));
                            break;

        case CELL_RADIO_ON:
                            {
                                if (radioOn)
                                {
                                    cell = SharedPointer<UIView>(new UITextView("Radio: On", &Font_Menu));
                                }
                                else
                                {
                                    cell = SharedPointer<UIView>(new UITextView("Radio: Off", &Font_Menu));
                                }

                                radioCell = cell;
                            }
                            break;

        case CELL_RADIO_CONNECTED:
                            {
//        cell = new UITextMonitorView<bool>(&connected, "Connected: %d", &Font_Menu);
                                if (connected)
                                {
                                    cell = SharedPointer<UIView>(new UITextView("Connected", &Font_Menu));
                                }
                                else
                                {
                                    cell = SharedPointer<UIView>(new UITextView("Disconnected", &Font_Menu));
                                }
                            }
                            break;

        // bottom filler
        case CELL_END_FILLER:
        default:
                            cell = SharedPointer<UIView>(new UIImageView(NULL));
                            break;
    }

    cell->setHorizontalAlignment(UIView::ALIGN_LEFT);
    cell->setVerticalAlignment(UIView::VALIGN_MIDDLE);
    cell->setWidth(0);
    cell->setHeight(0);

    return cell;
}

uint32_t SettingsMenuTable::getSize() const
{
    return CELL_END_FILLER + 1;
}

uint32_t SettingsMenuTable::widthAtIndex(uint32_t index) const
{
    (void) index;

    return 128;
}

uint32_t SettingsMenuTable::heightAtIndex(uint32_t index) const
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

SharedPointer<UIView::Action> SettingsMenuTable::actionAtIndex(uint32_t index)
{
    SharedPointer<UIView::Action> returnObject = SharedPointer<UIView::Action>(new UIView::Action(UIView::Action::None));

    switch (index)
    {
        case CELL_SET_TIME:
            {
                SharedPointer<UIView::Array> table(new SetTimeMenuTable());
                returnObject = SharedPointer<UIView::Action>(new UIView::Action(table));
            }
            break;

        case CELL_RADIO_ON:
            {
                // invalidate UIView cell
                radioCell->invalidate();

                // send message to radio
                sendBlock = SharedPointer<BlockStatic>(new BlockDynamic(4));
                Cbore encoder(sendBlock->getData(), sendBlock->getLength());

                if (radioOn)
                {
                    radioOn = false;

                    encoder.array(2)
                           .item(1)
                           .item(0);
                }
                else
                {
                    radioOn = true;

                    encoder.array(2)
                           .item(1)
                           .item(1);
                }

                sendBlock->setLength(encoder.getLength());

                FunctionPointer0<void> fp(this, &SettingsMenuTable::sendTaskDone);

                MessageCenter::sendTask(MessageCenter::RemoteHost,
                                        MessageCenter::RadioPort,
                                        *sendBlock.get(),
                                        fp);
            }
            break;

        default:
            break;
    }

    return returnObject;
}

void SettingsMenuTable::sendTaskDone()
{
    // free block
    sendBlock = SharedPointer<BlockStatic>();
}

const char* SettingsMenuTable::getTitle() const
{
    return "menu:settings";
}

uint32_t SettingsMenuTable::getFirstIndex() const
{
    return CELL_TOP_FILLER + 1;
}

uint32_t SettingsMenuTable::getLastIndex() const
{
    return CELL_END_FILLER - 2;
}

uint32_t SettingsMenuTable::getDefaultIndex() const
{
    return CELL_SET_TIME;
}
