/*
 * GUI widgets for shell scripts - dialogbox version 1.0
 *
 * Copyright (C) 2015-2016, 2020 Andriy Martynets <andy.martynets@gmail.com>
 *------------------------------------------------------------------------------
 * This file is part of dialogbox.
 *
 * Dialogbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dialogbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dialogbox. If not, see http://www.gnu.org/licenses/.
 *------------------------------------------------------------------------------
 */

#include "dialogbox.h"

using namespace DialogCommandTokens;

DialogParser::DialogParser(DialogBox *parent, FILE *in) :
    QThread(parent),
    dialog(parent),
    input(in)
{
    command = NoopCommand;
    control = WidgetMask;
    stage = StageCommand;
    token = bufferIndex = 0;

    qRegisterMetaType<DialogCommand>("DialogCommand");
    // Qt::BlockingQueuedConnection type is used to ensure commands are executed
    // sequentially. This avoids races e.g. show hide show sequence in v1.0.
    if (parent) {
        connect(this, SIGNAL(sendCommand(DialogCommand)), parent,
                SLOT(executeCommand(DialogCommand)),
                Qt::BlockingQueuedConnection);
    }
}

DialogParser::~DialogParser()
{
    // We cannot gracefully terminate this thread as input from stdin may be
    // blocking when attached to a terminal.
    terminate();
    wait();
}

void DialogParser::setParent(DialogBox *parent)
{
    if (dialog) {
        disconnect(this, SIGNAL(sendCommand(DialogCommand)),
                   dialog, SLOT(executeCommand(DialogCommand)));
    }

    dialog = parent;
    QObject::setParent(parent);
    if (parent) {
        connect(this, SIGNAL(sendCommand(DialogCommand)), parent,
                SLOT(executeCommand(DialogCommand)),
                Qt::BlockingQueuedConnection);
    }
}

void DialogParser::run()
{
    bool quoted;
    bool backslash;
    bool endOfLine;
    int c;

    // This thread will be ended up by terminate() call from the destructor.
    while (true) {
        quoted = backslash = endOfLine = false;
        do {
            if (bufferIndex != token)
                token = ++bufferIndex;

            while ((c = fgetc(input)) != EOF) {
                if (isspace(c) && !quoted) {
                    if (isblank(c)) {
                        if (bufferIndex == token)
                            continue;
                        else
                            break;
                    } else {
                        if (!backslash) {
                            endOfLine = true;
                            break;
                        } else {
                            backslash = false;
                            continue;
                        }
                    }
                }
                if (c == '"' && bufferIndex == token && !quoted && !backslash) {
                    quoted = true;
                    continue;
                }
                if (c == '"' && quoted && !backslash) {
                    quoted = false;
                    break;
                }
                if (c == '\\' && !backslash) {
                    backslash = true;
                    continue;
                }
                if (backslash && c != '"')
                    buffer[bufferIndex++] = '\\';
                backslash = false;
                buffer[bufferIndex++] = c;

                // We need to reserve space for backslash and terminating zero.
                if (bufferIndex >= BUFFER_SIZE - 2)
                    break;
            }
            if (backslash)
                buffer[bufferIndex++] = '\\';
            buffer[bufferIndex] = '\0';
            processToken();
            if (endOfLine) {
                issueCommand();
                endOfLine = false;
            }
        } while (c != EOF);
        issueCommand();
        // Sleep for a while to reduce the CPU time consumption.
        msleep(50);
    }
}

/*******************************************************************************
 *  DialogParser::processToken analyses tokens and assembles commands of them.
 ******************************************************************************/
void DialogParser::processToken()
{
    const struct {
        const char *commandKeyword;  // keyword to recognize
        unsigned int commandCode;    // command code to assign
        unsigned int commandStages;  // stages set to assign
    } commandsParser[] = {
        {"add", AddCommand,
            StageType | StageTitle | StageName | StageOptions | StageText
            | StageAuxText | StageCommand},
        {"end", EndCommand, StageType | StageCommand},
        {"position", PositionCommand, StageOptions | StageText | StageCommand},
        {"remove", RemoveCommand, StageName | StageCommand},
        {"clear", ClearCommand, StageName | StageCommand},
        {"step", StepCommand, StageOptions | StageCommand},
        {"set", SetCommand,
            StageName | StageOptions | StageText | StageCommand},
        {"unset", UnsetCommand, StageName | StageOptions | StageCommand},
        {"enable", SetCommand | (OptionEnabled & OptionMask),
            StageName | StageCommand},
        {"disable", UnsetCommand | (OptionEnabled & OptionMask),
            StageName | StageCommand},
        {"show", SetCommand | (OptionVisible & OptionMask),
            StageName | StageCommand},
        {"hide", UnsetCommand | (OptionVisible & OptionMask),
            StageName | StageCommand},
        {"query", QueryCommand, StageCommand},
        {nullptr, 0, 0}
    };

    const struct {
        const char *controlKeyword;  // keyword to recognize
        unsigned int controlCode;    // control type code to assign
    } controlsParser[] = {
        {"checkbox", CheckBoxWidget},
        {"frame", FrameWidget},
        {"groupbox", GroupBoxWidget},
        {"label", LabelWidget},
        {"pushbutton", PushButtonWidget},
        {"radiobutton", RadioButtonWidget},
        {"separator", SeparatorWidget},
        {"textbox", TextBoxWidget},
        {"listbox", ListBoxWidget},
        {"dropdownlist", ComboBoxWidget},
        {"combobox", ComboBoxWidget | (PropertyEditable & PropertyMask)},
        {"item", ItemWidget},
        {"progressbar", ProgressBarWidget},
        {"slider", SliderWidget},
        {"textview", TextViewWidget},
        {"tabs", TabsWidget},
        {"page", PageWidget},
        {nullptr, 0}
    };

    // Each item of the array below defines properties or options of a control.
    // If the same keyword applies to different properties/options it must be
    // repeated as a separate item.
    const struct {
        const char *optionKeyword;  // keyword to recognize
        unsigned int optionCode;    // command option or control property to set
        bool optionReset;           // flag to reset the option
        bool commandFlag;           // flag to process command option
    } optionsParser[] = {
        {"checkable", PropertyCheckable, false, false},
        {"checked", PropertyChecked, false, false},
        {"text", PropertyText, false, false},
        {"title", PropertyTitle, false, false},
        {"password", PropertyPassword, false, false},
        {"placeholder", PropertyPlaceholder, false, false},
        {"icon", PropertyIcon, false, false},
        {"iconsize", PropertyIconSize, false, false},
        {"animation", PropertyAnimation, false, false},
        {"picture", PropertyPicture, false, false},
        {"apply", PropertyApply, false, false},
        {"exit", PropertyExit, false, false},
        {"default", PropertyDefault, false, false},
        // space and stretch are a kind of controls without options
        {"space", OptionSpace, false, true},
        {"stretch", OptionStretch, false, true},
        {"behind", OptionBehind, false, true},
        {"onto", OptionOnto, false, true},
        {"enabled", OptionEnabled, false, true},
        {"focus", OptionFocus, false, true},
        {"stylesheet", OptionStyleSheet, false, true},
        {"visible", OptionVisible, false, true},
        {"horizontal", OptionVertical, true, true},
        {"horizontal", PropertyVertical, true, false},
        {"vertical", OptionVertical, false, true},
        {"vertical", PropertyVertical, false, false},
        {"plain", PropertyPlain, false, false},
        {"raised", PropertyRaised, false, false},
        {"sunken", PropertySunken, false, false},
        {"noframe", PropertyNoframe, false, false},
        {"box", PropertyBox, false, false},
        {"panel", PropertyPanel, false, false},
        {"styled", PropertyStyled, false, false},
        {"current", PropertyCurrent, false, false},
        {"activation", PropertyActivation, false, false},
        {"selection", PropertySelection, false, false},
        {"minimum", PropertyMinimum, false, false},
        {"maximum", PropertyMaximum, false, false},
        {"value", PropertyValue, false, false},
        {"busy", PropertyBusy, false, false},
        {"file", PropertyFile, false, false},
        {"top", PropertyPositionTop, false, false},
        {"bottom", PropertyPositionBottom, false, false},
        {"left", PropertyPositionLeft, false, false},
        {"right", PropertyPositionRight, false, false},
        {nullptr, 0, false, false}
    };

    if (stage & StageCommand) {
        int i = 0;

        while (commandsParser[i].commandKeyword) {
            if (!strcmp(buffer + token, commandsParser[i].commandKeyword)) {
                issueCommand();

                command = commandsParser[i].commandCode;
                stage = commandsParser[i].commandStages;

                return;
            }

            i++;
        }
    }

    if (stage & StageType) {
        int i = 0;

        while (controlsParser[i].controlKeyword) {
            if (!strcmp(buffer + token, controlsParser[i].controlKeyword)) {
                control = controlsParser[i].controlCode;
                stage ^= StageType;

                // Make bufferIndex equal to token to discard current token.
                // Set them to zero to rewind to the beginning of the buffer.
                bufferIndex = token = 0;
                return;
            }

            i++;
        }
    }

    if (stage & StageOptions) {
        int i = 0;

        while (optionsParser[i].optionKeyword) {
            if (!strcmp(buffer + token, optionsParser[i].optionKeyword)) {
                if (optionsParser[i].commandFlag) {
                    if (optionsParser[i].optionCode & command) {
                        if (optionsParser[i].optionReset) {
                            // Reset option bit
                            command &= ~(optionsParser[i].optionCode
                                       & OptionMask);
                        } else {
                            // Set option bit
                            command |= optionsParser[i].optionCode & OptionMask;
                        }

                        stage &= ~(StageTitle | StageName);

                        // Make bufferIndex equal to token to discard current
                        // token
                        bufferIndex = token;
                        return;
                    }
                } else {
                    if (optionsParser[i].optionCode & control) {
                        // Make control type more specific
                        control &= optionsParser[i].optionCode | PropertyMask;

                        if (optionsParser[i].optionReset) {
                            // Reset property bit
                            control &= ~(optionsParser[i].optionCode
                                       & PropertyMask);
                        } else {
                            // Set property bit
                            control |= optionsParser[i].optionCode
                                       & PropertyMask;
                        }

                        stage &= ~(StageTitle | StageName);

                        // Make bufferIndex equal to token to discard current
                        // token
                        bufferIndex = token;
                        return;
                    }
                }
            }

            i++;
        }
    }

    if (stage & StageTitle) {
        title = token;

        stage ^= StageTitle;

        // Set token to be different from bufferIndex.
        // This indicates the token was recognized.
        // Next, values for bufferIndex and token will be set in run() function.
        token = BUFFER_SIZE;
        return;
    }

    if (stage & StageName) {
        name = token;
        stage ^= StageName;

        token = BUFFER_SIZE;
        return;
    }

    if (stage & StageText) {
        text = token;
        stage ^= StageText;

        token = BUFFER_SIZE;
        return;
    }
    if (stage & StageAuxText) {
        auxtext = token;
        stage ^= StageAuxText;

        token = BUFFER_SIZE;
        return;
    }

    // The case the token wasn't recognized
    bufferIndex = token;
}

void DialogParser::issueCommand()
{
    if (command != NoopCommand) {
        emit sendCommand(*this);

        command = NoopCommand;
        control = WidgetMask;
        stage = StageCommand;
        title = name = text = auxtext = BUFFER_SIZE - 1;
        token = bufferIndex = 0;
    }
}
