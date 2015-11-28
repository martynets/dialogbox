/*
 * GUI widgets for shell scripts - dialogbox version 0.8
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
	widgetcommand=noop;
	widgettype=none;
	widgetoptions=option_reset;
	widgetstage=stage_command;
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

	if(widgetstage & stage_command)
		{
			if(!strcmp(buffer+token,"add"))
				{
					issue_command();

					widgetcommand=add;
					widgetstage=stage_type | stage_title | stage_name | stage_options | stage_text | stage_aux_text | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"end"))
				{
					issue_command();

					widgetcommand=end;
					widgettype=groupbox; //???
					widgetstage=stage_type | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"step"))
				{
					issue_command();

					widgetcommand=step;
					widgettype=none;
					widgetstage=stage_options | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"set"))
				{
					issue_command();

					widgetcommand=set;
					widgetstage=stage_name | stage_options | stage_text | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"unset"))
				{
					issue_command();

					widgetcommand=unset;
					widgetstage=stage_name | stage_options | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"enable"))
				{
					issue_command();

					widgetcommand=enable;
					widgetstage=stage_name | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"disable"))
				{
					issue_command();

					widgetcommand=disable;
					widgetstage=stage_name | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"remove"))
				{
					issue_command();

					widgetcommand=remove;
					widgetstage=stage_name | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"position"))
				{
					issue_command();

					widgetcommand=position;
					widgetstage=stage_options | stage_text | stage_command;

					return;
				}
			if(!strcmp(buffer+token,"print"))
				{
					issue_command();

					widgetcommand=print;
					widgetstage=stage_command;

					return;
				}
		}

	if(widgetstage & stage_type)
		{
			if(!strcmp(buffer+token,"pushbutton"))
				{
					widgettype=pushbutton;
					widgetstage^=stage_type;

					// make buffer_index equal to token to discard current token
					// set them to zero to rewind to the beginning of the buffer
					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"checkbox"))
				{
					widgettype=checkbox;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"radiobutton"))
				{
					widgettype=radiobutton;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"groupbox"))
				{
					widgettype=groupbox;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"textbox"))
				{
					widgettype=textbox;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"label"))
				{
					widgettype=label;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"stretch"))
				{
					widgettype=stretch;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"space"))
				{
					widgettype=space;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
			if(!strcmp(buffer+token,"separator"))
				{
					widgettype=separator;
					widgetstage^=stage_type;

					buffer_index=token=0;
					return;
				}
		}

	if(widgetstage & stage_options)
		{
			if(!strcmp(buffer+token,"checked"))
				{
					widgetoptions|=option_checked;
					widgetstage&=~(stage_title | stage_name);

					// make buffer_index equal to token to discard current token
					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"checkable"))
				{
					widgetoptions|=option_checkable;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"enabled"))
				{
					widgetoptions|=option_enabled;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"password"))
				{
					widgetoptions|=option_password;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"vertical"))
				{
					widgetoptions|=option_vertical;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"horizontal"))
				{
					widgetoptions&=~option_vertical;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"picture"))
				{
					widgetoptions|=option_picture;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"animation"))
				{
					widgetoptions|=option_animation;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"apply"))
				{
					widgetoptions|=option_apply;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"exit"))
				{
					widgetoptions|=option_exit;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"default"))
				{
					widgetoptions|=option_default;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"focus"))
				{
					widgetoptions|=option_focus;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"title"))
				{
					widgetoptions|=option_title;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"icon"))
				{
					widgetoptions|=option_icon;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"text"))
				{
					widgetoptions|=option_text;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"placeholder"))
				{
					widgetoptions|=option_placeholder;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"behind"))
				{
					widgetoptions|=option_behind;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"onto"))
				{
					widgetoptions|=option_onto;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"iconsize"))
				{
					widgetoptions|=option_iconsize;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
			if(!strcmp(buffer+token,"stylesheet"))
				{
					widgetoptions|=option_stylesheet;
					widgetstage&=~(stage_title | stage_name);

					buffer_index=token;
					return;
				}
		}

	if(widgetstage & stage_title)
		{
			widgettitle=token;

			widgetstage^=stage_title;

			// set token to be different from buffer_index
			// this indicates the token was recognized
			// next values for buffer_index and token will be set in run() function
			token=BUFFER_SIZE;
			return;
		}

	if(widgetstage & stage_name)
		{
			widgetname=token;
			widgetstage^=stage_name;

			token=BUFFER_SIZE;
			return;
		}

	if(widgetstage & stage_text)
		{
			widgettext=token;
			widgetstage^=stage_text;

			token=BUFFER_SIZE;
			return;
		}
	if(widgetstage & stage_aux_text)
		{
			widgetauxtext=token;
			widgetstage^=stage_aux_text;

			token=BUFFER_SIZE;
			return;
		}

	// the case the token wasn't recognized
	buffer_index=token;
}

void DialogParser::issue_command()
{
	if(widgetcommand!=noop)
		{
			emit SendCommand(*this);

			widgetcommand=noop;
			widgettype=none;
			widgetoptions=option_reset;
			widgetstage=stage_command;
			widgettitle=widgetname=widgettext=widgetauxtext=BUFFER_SIZE-1;
			token=buffer_index=0;
		}
}
