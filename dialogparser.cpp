/*
 * GUI widgets for shell scripts - dialogbox version 0.9
 *
 * Copyright (C) 2015 Andriy Martynets <martynets@volia.ua>
 *--------------------------------------------------------------------------------------------------------------
 * This file is part of dialogbox.
 *
 * Dialogbox is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * Dialogbox is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with dialogbox.
 * If not, see http://www.gnu.org/licenses/.
 *--------------------------------------------------------------------------------------------------------------
 */

#include "dialogbox.hpp"

DialogParser::DialogParser(DialogBox* parent, FILE* in) :
			QThread(parent),
			dialog(parent),
			input(in)
{
	command=noop;
	control=widget_mask;
	stage=stage_command;
	token=buffer_index=0;

	qRegisterMetaType<DialogCommand>("DialogCommand");
	if(parent)
		connect(this, SIGNAL(SendCommand(DialogCommand)),
			parent, SLOT(ExecuteCommand(DialogCommand)));
}

DialogParser::~DialogParser()
{
	terminate();	// we cannot gracefully terminate this thread as input
					//from stdin may be blocking when attached to a terminal
	wait();
}

void DialogParser::setParent(DialogBox* parent)
{
	if(dialog)
		disconnect(this, SIGNAL(SendCommand(DialogCommand)),
			dialog, SLOT(ExecuteCommand(DialogCommand)));

	dialog=parent;
	QObject::setParent(parent);
	if(parent)
		connect(this, SIGNAL(SendCommand(DialogCommand)),
			parent, SLOT(ExecuteCommand(DialogCommand)));
}

void DialogParser::run()
{
	bool quoted, backslash, endofline;
	int c;

	while(true) // this thread will be ended up by terminate() call from the destructor
		{
			quoted=backslash=endofline=false;
			do
				{
					if(buffer_index!=token) token=++buffer_index;

					while ((c=fgetc(input))!=EOF)
						{
							if(isspace(c) && !quoted)
								{
									if(isblank(c))
										{
											if(buffer_index==token) continue;
											else break;
										}
									else
										{
											if(!backslash) { endofline=true; break; }
											else { backslash=false; continue; }
										}
								}
							if(c=='"' && buffer_index==token && !quoted && !backslash) { quoted=true; continue; }
							if(c=='"' && quoted && !backslash) { quoted=false; break; }
							if(c=='\\' && !backslash) { backslash=true; continue; }
							if(backslash && c!='"') buffer[buffer_index++]='\\';
							backslash=false;
							buffer[buffer_index++]=c;

							if(buffer_index>=BUFFER_SIZE-2) break;	// potentially we need space
																	// for backslash and terminating zero
						}
					if(backslash) buffer[buffer_index++]='\\';
					buffer[buffer_index]='\0';
					process_token();
					if(endofline)
						{
							issue_command();
							endofline=false;
						}
				}
			while(c!=EOF);
			issue_command();
			msleep(50);				// this call is to reduce the CPU time consumption
		}
}

/*******************************************************************************
 *
 *	Analyses tokens and assembles commands of them.
 * 	Commands recognized are:
 * 		add type title [name] [options] [text] [auxtext]
 * 		end [type]
 * 		step [options]
 * 		set [name] options [text]
 * 		unset [name] options
 * 		enable [name]
 * 		disable [name]
 * 		remove name
 * 		print
 * 		position options name (behind | onto)
 *
 * ****************************************************************************/
void DialogParser::process_token()
{
	const struct
		{
			const char* command_keyword;	// keyword to recognize
			unsigned int command_code;		// command code to assign
			unsigned int command_stages;	// stages set to assign
		} commands_parser[]=
		{
			{"add", add, stage_type | stage_title | stage_name | stage_options | stage_text | stage_aux_text | stage_command},
			{"enable", enable, stage_name | stage_command},
			{"end", end, stage_type | stage_command},
			{"disable", disable, stage_name | stage_command},
			{"position", position, stage_options | stage_text | stage_command},
			{"remove", remove, stage_name | stage_command},
			{"set", set, stage_name | stage_options | stage_text | stage_command},
			{"step", step, stage_options | stage_command},
			{"unset", unset, stage_name | stage_options | stage_command},
			{"print", print, stage_command},
			{NULL, 0, 0}
		};

	const struct
		{
			const char* control_keyword;	// keyword to recognize
			unsigned int control_code;		// control type code to assign
		} controls_parser[]=
		{
			{"checkbox", checkbox},
			{"frame", frame},
			{"groupbox", groupbox},
			{"label", label},
			{"pushbutton", pushbutton},
			{"radiobutton", radiobutton},
			{"separator", separator},
			{"space", space},
			{"stretch", stretch},
			{"textbox", textbox},
			{NULL, 0}
		};

	// each struct in the array below must define properties for the same control type or set of types
	// if the same keyword applies to different properties/control types it must be repeated as new struct
	const struct
		{
			const char* option_keyword;		// keyword to recognize
			unsigned int option_code;		// command option/control property to set or reset
			bool option_reset;				// flag to reset the option
			bool command_flag;				// flag to process command option
		} options_parser[]=
		{
			{"checkable", property_checkable, false, false},
			{"checked", property_checked, false, false},
			{"text", property_text, false, false},
			{"title", property_title, false, false},
			{"password", property_password, false, false},
			{"placeholder", property_placeholder, false, false},
			{"icon", property_icon, false, false},
			{"iconsize", property_iconsize, false, false},
			{"animation", property_animation, false, false},
			{"picture", property_picture, false, false},
			{"apply", property_apply, false, false},
			{"exit", property_exit, false, false},
			{"default", property_default, false, false},
			{"behind", option_behind, false, true},
			{"onto", option_onto, false, true},
			{"enabled", option_enabled, false, true},
			{"focus", option_focus, false, true},
			{"stylesheet", option_stylesheet, false, true},
			{"horizontal", option_vertical, true, true},
			{"horizontal", property_vertical, true, false},
			{"vertical", option_vertical, false, true},
			{"vertical", property_vertical, false, false},
			{"plain", property_plain, false, false},
			{"raised", property_raised, false, false},
			{"sunken", property_sunken, false, false},
			{"noframe", property_noframe, false, false},
			{"box", property_box, false, false},
			{"panel", property_panel, false, false},
			{"styled", property_styled, false, false},
			{NULL, 0, 0, 0}
		};

	if(stage & stage_command)
		{
			int i=0;

			while(commands_parser[i].command_keyword)
				{
					if(!strcmp(buffer+token,commands_parser[i].command_keyword))
						{
							issue_command();

							command=commands_parser[i].command_code;
							stage=commands_parser[i].command_stages;

							return;
						}

					i++;
				}
		}

	if(stage & stage_type)
		{
			int i=0;

			while(controls_parser[i].control_keyword)
				{
					if(!strcmp(buffer+token,controls_parser[i].control_keyword))
						{
							control=controls_parser[i].control_code;
							stage^=stage_type;

							// make buffer_index equal to token to discard current token
							// set them to zero to rewind to the beginning of the buffer
							buffer_index=token=0;
							return;
						}

					i++;
				}
		}

	if(stage & stage_options)
		{
			int i=0;

			while(options_parser[i].option_keyword)
				{
					if(!strcmp(buffer+token,options_parser[i].option_keyword))
						{
							if(options_parser[i].command_flag)
								{
									if(options_parser[i].option_code & command)
										{
											if(options_parser[i].option_reset)
												command&=~(options_parser[i].option_code & option_mask);	// reset option bit
											else
												command|=options_parser[i].option_code & option_mask;	// set option bit

											stage&=~(stage_title | stage_name);

											// make buffer_index equal to token to discard current token
											buffer_index=token;
											return;
										}
								}
							else
								{
									if(options_parser[i].option_code & control)
										{
											// make control type more specific
											control&=options_parser[i].option_code | property_mask;

											if(options_parser[i].option_reset)
												control&=~(options_parser[i].option_code & property_mask);	// reset property bit
											else
												control|=options_parser[i].option_code & property_mask;	// set property bit

											stage&=~(stage_title | stage_name);

											// make buffer_index equal to token to discard current token
											buffer_index=token;
											return;
										}
								}
						}

					i++;
				}
		}

	if(stage & stage_title)
		{
			title=token;

			stage^=stage_title;

			// set token to be different from buffer_index
			// this indicates the token was recognized
			// next values for buffer_index and token will be set in run() function
			token=BUFFER_SIZE;
			return;
		}

	if(stage & stage_name)
		{
			name=token;
			stage^=stage_name;

			token=BUFFER_SIZE;
			return;
		}

	if(stage & stage_text)
		{
			text=token;
			stage^=stage_text;

			token=BUFFER_SIZE;
			return;
		}
	if(stage & stage_aux_text)
		{
			auxtext=token;
			stage^=stage_aux_text;

			token=BUFFER_SIZE;
			return;
		}

	// the case the token wasn't recognized
	buffer_index=token;
}

void DialogParser::issue_command()
{
	if(command!=noop)
		{
			emit SendCommand(*this);

			command=noop;
			control=widget_mask;
			stage=stage_command;
			title=name=text=auxtext=BUFFER_SIZE-1;
			token=buffer_index=0;
		}
}
