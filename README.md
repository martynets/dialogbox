Copyright (C) 2015 Andriy Martynets [martynets@volia.ua](mailto:martynets@volia.ua)<br>
See the end of the file for license conditions.

-------------------------------------------------------------------------------
#####Сontents
- [Introduction](#introduction)
- [Software Requirements](#software-requirements)
- [Downloading](#downloading)
- [Installation](#installation)
- [Usage](#usage)
- [Widgets](#widgets)
- [Layouts](#layouts)
- [Commands](#commands)
	- [Commands syntax and description](#commands-syntax-and-description)
- [Invocations](#invocations)
	- [Case 1: User accepts or rejects the dialog box](#case-1-user-accepts-or-rejects-the-dialog-box)
	- [Case 2: User enters data and accepts the dialog](#case-2-user-enters-data-and-accepts-the-dialog)
	- [Case 3: User input requires the dialog box to be modified (pipes)](#case-3-user-input-requires-the-dialog-box-to-be-modified-pipes)
	- [Case 4: User input requires the dialog box to be modified (FIFOs)](#case-4-user-input-requires-the-dialog-box-to-be-modified-fifos)
	- [Case 5: Manage a background process with GUI frontend](#case-5-manage-a-background-process-with-gui-frontend)
- [Examples](#examples)
- [Bug Reporting](#bug-reporting)
- [Change Log](#change-log)
- [License](#license)

-------------------------------------------------------------------------------

#####Introduction
The `dialogbox` application is a scriptable GUI dialod box with various widgets. The desired dialog box is built and modified by commands read on the standard input when the end-user actions are reported on standard output.

The main purpose of this application is to provide GUI functionality to shell scripts or any other pure console applications capable to communicate with a child process via its standard input/output.

This is a Qt application and it introduces all the power of Qt to shell scripts from command buttons through stylesheets and animations.

#####Software Requirements
This application is designed using the Qt 4.8 library and this is the only dependency. It must be portable across systems supported by the Qt but the author tested it in GNU/Linux environment only.

To compile the application from the source code the following packages must also be installed on a GNU/Linux system:
- g++
- make
- libqt4-dev

#####Downloading
The latest version of the `dialogbox` application can be downloaded from the link below:

https://github.com/martynets/dialogbox/releases/latest

The `dialogbox` project is also available via Git access from the GitHub server:

https://github.com/martynets/dialogbox.git

#####Installation
To compile and install the application issue the following commands from the source directory:
```
qmake
make
make install
```
> Note: the application is installed in `/usr/bin` directory and thus the last one command requires root privileges

To uninstall the application issue the following command from the same directory (the same note is applicable here):
```
make uninstall
```
#####Usage
The application reads commands on its standard input, builds and modifies the dialog box based on the commands provided, interacts with the end-user and reports the user's actions on the standard output and by the exit status. The dialog box can be either accepted or rejected by the user. This is reflected by the exit status of the application. If the user accepted the dialog box all enabled modifiable widgets which have names are reported to the standard output by pairs `<widget_name>=<value>`, one per line.

Pushbuttons (command buttons) are reported immediately once they are clicked in form `<pushbutton_name>=clicked`. A pushbutton can have `exit` option. In this case clicking this button terminates the application with rejected status. It can also have `apply` option. Clicking it in this case produses output for accepted dialog described above. Clicking a pushbutton with both these options terminates the application with accepted status. This is the only way to accept the dialog box. Closing the application using the Window Manager's controls or pressing `Esc` key rejects the dialog box.

The application recognizes several command line options. The command line syntax is the following:
```
dialogbox  [OPTIONS]
```
Options recognized are the [standard Qt options](http://doc.qt.io/qt-4.8/qapplication.html#QApplication) and the application specific ones described below:

|Option|Action|
|------|------|
|-h, --help|display brief usage information and exit|
|-v, --version|display version information and exit|

######Values of exit status mean the following:
- 0 - success (`--help` or `--version` only)
- 0 - user rejected the dialog (closed the window using the WM controls, pressed `Esc` key or clicked a pushbutton with `exit` option set and `apply` option unset)
- 1 - user accepted the dialog (clicked a pushbutton with both `apply` and `exit` options set)

#####Widgets
Current version of the `dialogbox` supports the following widget types:
- **pushbutton** (command button) - a control which can be clicked to initiate an action. It can have title (text), icon, can be checkable (pressed or depressed), can have `exit` and `apply` options described above. One of the pushbuttons within the dialog box can optionally be set as the default one. That means it is clicked once the end-user hits `Enter` key.<br>
Pushbutton widgets are reported immediately as they are clicked with value of `clicked`.
- **checkbox** (tick box) - a control which can be clicked to make it checked or unchecked. That indicates the option the control represents is either on or off. This control can have title (text) and optional icon.<br>
Value of checkbox widget is reported as `1` if checked and as `0` if unchecked.

	>Note: the icon for this control is an extra icon for the text label. Don't mix it with the check mark icon. To customize the latter use stylesheets.

- **radiobutton** - a clickable control to select an option represented by it within group of similar controls that represent alternative options. These controls are used by groups to represent a selection of options when only one of them is possible at a time. As the previous control this one can have title (text) and optional icon.<br>
Value of radiobutton widget is reported as `1` if it is the selected one and as `0` if it is not.
- **groupbox** - a container control which provides visual separation and groupping of another widgets. It might have border or be flat depending on current theme. Optionally this control can have title (text) and can be checkable (has checkbox sub-control). All these options head the group of child widgets. If groupbox is checkable and is unchecked all child widgets are disabled. This type of widget hosts layout of either horizontal or vertical  type which manages sizing and positioning of child widgets. See [Layouts](#layouts) section below.<br>
Value of groupbox widget is reported as `1` if it is checkable and is checked and as `0` otherwise.

- **textbox** (line edit, edit field) - a one line text edit control. A widget of this type can have title, text, placeholder text and password options. The title is a text label in front to name the value edited. The text is the value edited itself. The placeholder is a hint text shown instead of empty text. The password option is a flag which instructs the control to hide the value entered showing asterisks instead of the characters.<br>
Reported value of textbox widget is the text entered by the end-user.
- **label** - an informative control displaying content of either text, picture or animation type. For text content both plain and reach text formats can be used. As the picture an image file in one of the following formats can be used:

	|Format|Description|
	|------|-----------|
	|BMP|Windows Bitmap|
	|GIF|Graphic Interchange Format|
	|JPG|Joint Photographic Experts Group|
	|JPEG|Joint Photographic Experts Group|
	|PNG|Portable Network Graphics|
	|PBM|Portable Bitmap|
	|PGM|Portable Graymap|
	|PPM|Portable Pixmap|
	|XBM|X11 Bitmap|
	|XPM|X11 Pixmap|

	The animation is also a file in either Animated GIF or Multiple-image Network Graphics (MNG) format.<br>
Label widget alters its layout's alignment. For text label it is set to top-left which is the default alignment for layouts. For picture and animation labels alignment is set to centered in both directions. This could confuse if another widgets are added to the same layout. It is advised to keep picture and animation labels on separate layouts.<br>
Label widgets are not reported.
- **separator** - a decorative control which forms a border to split the dialog box into logical areas. The control is a line of sunken shadow either horizontal or vertical orientation.<br>
Separator widgets are not reported.

	>Note: separator widget is fully shrinkable and as the result a vertical separator has zero height on vertical layout if the latter contains more widgets. The same is true for length of horizontal separator on a horizontal layout with more widgets. To make them visible use them solely on a separate layouts of the same orientation or use vertical separators on horizontal layouts and horizontal separators on vertical layouts depending on design of the dialog box.

For better widgets positioning two types of spacer items are supported:
- **stretch** - locates free space available within the layout

	>Note: a stretch is noticeable only if there is free space available on the layout. This can be a case if the vertical layout has higher neighbor or the horizontal one has broader neighbor.

- **space** - adds extra space in pixels

Navigation between widgets can be done using a pointing device (e.g. mouse) or by keyboard. The latter way consists of tabbing and shortcuts. The tabbing order is setup based on widgets position withing a layout and layouts within the main window. This includes selectable, editable and navigation widgets. Text labels are included by default as they might contain links - navigation controls.

>Tip: to exclude a text label from the tabbing order set its stylesheet with the following property:<br>
>`qproperty-textInteractionFlags: NoTextInteraction;`

Keyboard shortcuts can be added to widgets by preceding a desired character with ampersand symbol in the widget's title. The character then is displayed underlined. Pressing keyboard combination `Alt-<character>` moves focus to that widget.

>Tip: use `&&` to display an actual ampersand.

Widgets can be added, modified, styled, disabled, enabled and removed. See [Commands](#commands) section below. All actions taken on widgets are done using their names which are optionally setup when widgets are added.

> Note: if a widget misses name it can not be modified, removed or reported.<br>
> Only enabled widgets are reported.

Some commands that modify existing widgets can be used without widget name specified. In this case they are addressed to the whole dialog box and affect either the main window or all widgets at once.

Widgets can be styled using stylesheets. This technic allows to customize spacing, sizing, colors, background, fonts, images, etc. for widgets and their sub-controls. Stylesheets can be setup for a particular widget, group of widgets, class of widgets and all widgets including main window. See [Qt Style Sheet](http://doc.qt.io/qt-4.8/stylesheet-syntax.html) for more information.

Below screenshot demonstrates supported widget types:

![Widget types](./images/widget-types-demo.png)

>More widget types are scheduled for the release version of the `dialogbox` application.

#####Layouts
Complex dialog box design is based on nested layouts structure. Layouts are responsible for positioning and sizing of their child objects. Horizontal layouts position their objects from left to right when vertical layouts from top to bottom. Combination of horizontal and vertical layouts allows to freely position and logically group any widgets to create desired dialog box.

The `dialogbox` application maintains three level layouts structure. It consists of main vertical layout which hosts horizontal layouts which in turn host the final third level of vertical layouts. Only the last one hosts widgets. Initially there is only one layout on each level. On demand they are added on the third and on the second levels. One of the third level layouts is the current one. That is the layout where widgets are added.

Widgets are added as a column. When there is a need to add new controls at a side, the application does "horizontal step" by adding new vertical layout on the third level and makes it the current one. To start new composition of widgets below the application does "vertical step" by adding new horizontal layout on the second level with single vertical layout on the third level which becomes the current one. And so forth.

To summarize, the main layout, level 1, is to support move vertically, horizontal layouts, level 2, are to support move horizontally and the level 3 is to host target widgets.

The described above nested structure of layouts is created by two commands only: `step horizontal` and `step vertical`. Additionally `position` command is supported to change the current layout.

>Note: empty layout (contains nothing except spacer items) is removed automatically as soon as it losts focus (is no longer the current one).

Some widgets (e.g. groupbox) can host own layout either vertical or horizontal. Adding such a widget makes its layout the current one and subsequent new widgets are added to it. The `end` command finishes this process and returns focus to the third level layout.

Below chart demonstrates example structure of layouts for a dialog:

![Nested Layouts structure](./images/layouts.png)

Next sequence of meta commands builds a dialog box with the above structure:
```
add widget1
add widget2
step horizontal
add widget3
step vertical
add groupbox widget4
add stretch
add widget5
add widget6
end groupbox
add widget7
```
The alive example with the above structure might look the following:

![Layouts example](./images/layouts-example.png)

#####Commands
The `dialogbox` reads commands from its standard input. Each command consists of one or more tokens. It is started by a token of action followed by optional tokens of type/name/options and is ended by next command, end of line or end of file. A command can be continued on next line if the end of line is escaped by back slash character (`\`). Commands are recognized on best efforts basis which means the application does what it understood. Everything that wasn't understood is silently ignored.

Tokens are separated by whitespaces. If a token contains a white space character (including new line) it must be quoted by double quote characters (`"`). To use double quote character within a token (either quoted or not) the former must be escaped by back slash character (`\`).

>Note: back slash character is interpreted specially only in two cases mentioned above. In all the rest cases it is interpreted literally.

Tokens can be either keywords (reserved words) or custom strings. The latter are used to define widgets' name, title, text and auxiliary text depending on command. Keywords are used to define commands themselves, widget types and options available. Tokens are case sensitive and keywords are always lowercase. Using keywords in positions where custom strings are expected is ill-advised and may lead to unpredictable results.

List of keywords reserved by current version of the `dialogbox` application:

|Commands|Controls|Options|Options|
|--------|--------|-------|-------|
|add|checkbox|animation|icon|
|enable|groupbox|apply|iconsize|
|end|label|behind|onto|
|disable|pushbutton|checkable|password|
|position|radiobutton|checked|picture|
|remove|separator|default|placeholder|
|set|space|enabled|stylesheet|
|step|stretch|exit|text|
|unset|textbox|focus|title|
|||horizontal|vertical|

>Note: for each command total size of all custom strings plus size of last token if it is a keyword should not exceed 1024 bytes. This includes terminating zeros for each of these tokens.

######Commands syntax and description:

- **`add type [title] [name] [options] [text] [auxtext]`** - adds control of type `type` at current position. Normally current position is bottom of current layout (see `Layouts` section above). This can be changed by `position` command. The rest arguments of the command are optional and vary depending on the type of the control. For any type of widget, `name`, if provided, is an unique identifier for the added widget and is used to refer to it in subsequent commands as well as to report its value.
	- `add checkbox [title] [name] [options]`<br>
		`title` - text used on the checkbox widget.<br>
        `options` - optional `checked` keyword which makes the checkbox checked.
	- `add groupbox [title] [name] [options]` - starts container widget. Subsequent `add` commands will add controls to this container. It is ended by either `end` or `step` or `add groupbox` or `position` commands.<br>
		`title` - heading title for the groupbox. Its style and decoration as well as visibility and style of the groupbox border depend on current theme.<br>
        `options` - optional `vertical`, `horizontal`, `checkable` and `checked` keywords which can be used in any combination and in any order. The first two keywords define the type of the layout hosted by the groupbox widget when horizontal is the default one. `checkable` adds checkbox sub-control and `checked` makes it checked.

        >Note: by default checkable groupbox is unchecked and, thus, all its child widgets are disabled.

	- `add label [title] [name] [options]`<br>
		`title` - text, either plain or reach one, for text labels or file name for labels of animation and picture types.<br>
        `options` - optional `picture` or `animation` keywords which specify the type of the label. If none used text type assumed.
	- `add pushbutton [title] [name] [options]`<br>
		`title` - text used on the pushbutton widget.<br>
        `options` - optional `apply`, `exit` and `default` keywords which can be used in any combination and in any order.

        >Note: making a pushbutton the default one clears this option from the previously default one, if any.

	- `add radiobutton [title] [name] [options]`<br>
		`title` - text used on the radiobutton widget.<br>
        `options` - optional `checked` keyword which selects the radiobutton.
	- `add separator [name] [options]`<br>
        `options` - optional `vertical` or `horizontal` keyword which defines the orientation of the widget. By default it is oriented horizontally.<br>
	- `add space [size]`<br>
		`size` - string with value of integer type which is the size in pixels for this spacer item. If none mentioned `1` is assumed.<br>
	- `add stretch`<br>
		This kind of spacer item has no options and its orientation is defined by the layout it is placed on.<br>
	- `add textbox [title] [name] [options] [text] [auxtext]`<br>
		`title` - text used a text label in front of the textbox widget.<br>
        `options` - optional `password` keyword which changes the echo mode of the widget that asterisks are displayed instead of the characters entered.<br>
		`text` - optional initial value for the text to edit.<br>
		`auxtext` - optional text which is used as the placeholder if provided.
- **`enable [name]`** - enables the named widget or whole dialod box if `name` is omitted. Is a synonym to `set [name] enabled` command.

	>Note: enabling of previously disabled widget when its parent is also disabled will have no effect until the parent is enabled.

- **`end [type]`** - ends current container widget (e.g. groupbox). All subsequent `add` commands will add widgets onto the current layout of level 3. Optional `type` can be mentioned for better reading.
- **`disable [name]`** - disables the named widget or whole dialod box if `name` is omitted. Is a synonym to `unset [name] enabled` command.

- **`position [options] name`** - moves focus for `add` command - changes current layout and/or position within it. The focus goes before the named widget. If the focus moves to a container widget (e.g. groupbox) the end of its hosting layout becames the focus point once the container is ended (`end`, `step`, `add groupbox` or `position` commands).<br>
`options` - optional `behind` or `onto` keyword. `behind` directs the command to move the focus behind the named widget. This is useful to position onto the end of a layout. `onto` directs to position onto the end of the named container widget. This is useful to position onto empty container widget.

- **`remove name`** - removes the named widget. It also removes child controls in case of a container widget. If the widget is the last non-spacer item on its hosting layout and the latter isn't the current one the layout is removed as well.
- **`set [name] options [text]`** - sets various options for the named widget or for the main window if `name` is omitted.<br>
`options` - mandatory one or more keywords which define the actual options to set. Set of applicable options depends on the widget type and if inappropriate option is used it is silently ignored. Multiple options can be set by a single command with limitation that only one of them requires the extra `text` argument.<br>
`text` - optional string value which meaning varies upon the option to be set.<br>
Below is the list of possible keywords/options and their explanation:<br>
	- `animation` - changes type of label widget to animation. `text` is used as the name of file with the content.
	- `apply` - sets `apply` option for pushbutton widget.
	- `checkable` - makes groupbox or pushbutton widgets checkable.
	- `checked` - makes checkable widget (checkbox, checkable groupbox or checkable pushbutton) checked or radiobutton selected.
	- `default` - makes pushbutton widget the default one for the dialog box.
	- `enabled` - enables the named widget or whole dialod box. Is equal to `enable` command.
	- `exit` - sets `exit` option for pushbutton widget.
	- `focus` - moves keyboard focus to the widget.
	- `icon` - sets icon for either widget or main window. `text` is used as the name of icon file. This option makes sense for checkbox, pushbutton and radiobutton widgets only.
	- `iconsize` - sets a maximum size for icon in pixels. Uses `text` argument as an integer value to set. Default size is set by current theme. Smaller icons are not scaled.
	- `password` - sets the echo mode for textbox widget to password.
	- `picture` - changes type of label widget to picture. `text` is used as the name of file with the content.
	- `placeholder` - sets the value of the placeholder text for textbox widget. Uses `text` argument as the value to set.
	- `stylesheet`- sets stylesheet for either widget or whole dialod box. Uses `text` argument as the value to set. Stylesheet might contain styles for particular widget or for classes of widgets. If set for a container widget or for whole dialog box it might affect child widgets of a particular class. See [Qt Style Sheet](http://doc.qt.io/qt-4.8/stylesheet-syntax.html) for more information.

		>Note: new stylesheet completely replaces previously set value regardless which styles are setup.

	- `text` - sets the value of the text to edit for textbox widget. For all the rest types of widgets is a synonym to `title` option. It is more readable for label widgets rather than `title` as emphasizes change its type to text. Uses `text` argument as the value to set.
	- `title` - changes the title of either widget or main window. For label widget changes its type to text. Uses `text` argument as the value to set.

- **`step [direction]`** - adds new layouts to the dialog box structure. Optional `direction` defines the direction in which the structure is extended. It can be either `vertical` or `horizontal` keyword when the latter is assumed if none mentioned. Step in horizontal direction adds new vertical layout on the third level at the right from current layout (inserts if the current isn't the last one). Step in vertical direction adds (inserts) new horizontal layout on the second level below the parent of the current layout. The former is added new vertical layout on the third level which becomes the current one. See [Layouts](#layouts) section above for more details.
- **`unset [name] options`** - unsets various options for the named widget or for the main window if `name` is omitted. This command is similar to the `set` one but is to reset flag-like options or value of parameters for a widget. Thus its list of applicable options excludes marker-like options (`focus` and `default`) as well as options for which reset of value makes no sense (`iconsize`).
	- `animation` - changes type of label widget to animation with empty content.
	- `apply` - turns off `apply` option for pushbutton widget.
	- `checkable` - makes groupbox or pushbutton widgets non-checkable.
	- `checked` - makes checkable widget (checkbox, checkable groupbox or checkable pushbutton) unchecked or radiobutton unselected.
	- `enabled` - disables the named widget or whole dialod box. Is equal to `disable` command.
	- `exit` - turns off `exit` option for pushbutton widget.
	- `icon` - unsets icon for either widget or main window.
	- `password` - sets the echo mode for textbox widget to normal (characters entered are shown as they are).
	- `picture` - changes type of label widget to picture with empty content.
	- `placeholder` - resets the value of the placeholder text for textbox widget.
	- `stylesheet`- resets stylesheet for either widget or whole dialod box.

		>Note: this might affect child widgets if they were styled using class or name references.

	- `text` - resets the value of the text to edit for textbox widget. For all the rest types of widgets is a synonym to `title` option. It is more readable for label widgets rather than `title` as emphasizes change its type to text.
	- `title` - resets the title of either widget or main window. For label widget changes its type to text.

#####Invocations
The `dialogbox` application can be used in various ways depending on the complexity of the task. Below there are reviews of 5 typical cases of its usage in a script. The [demos directory](./demos/) contains scripts which demonstrate all these cases and can be used as templates to solve real tasks. Below reviews contain quotations from that scripts for the reader's convenience.

######Case 1: User accepts or rejects the dialog box
Scenario: the script needs user's input in form "yes or no" which in GUI is usually represented by pushbuttons Ok and Cancel.

The application is used to show a prompt using GUI widgets and the script analyses its exit status. No names for widgets are used as widgets are neither modified nor their values are analysed. The `dialogbox` application is run foreground as the script waits for the user to complete interaction with the dialog box. There is one pushbutton with both `apply` and `exit` options set to accept the dialog. Any other way to close the application means rejection.

```shell
dialogbox <<EODEMO
add label "Please confirm the operation"
add groupbox horizontal
add stretch
add pushbutton C&ontinue apply exit
add pushbutton C&ancel exit
end groupbox
EODEMO

if [ "$?" == "0" ]
then
	echo User rejected dialog
else
	echo User accepted dialog
fi

```
See full version of the script [here](./demos/demo1).

######Case 2: User enters data and accepts the dialog
Scenario: the script needs values entered by user. The user enters data and accepts the dialog. Rejection of the dialog means cancellation of the process.

The application is used to build a dialog box with user alterable options/fields and the script analyses its output. For this task unidirectional communication with the `dialogbox` application is suitable when its output is redirected to a loop where it is read and analysed. The loop ends once the application closes its output stream which happens when the user completes the interaction with the dialog.

```shell
flag=0

while IFS=$'=' read key value
do
	case $key in
		cb1)
			if [ "$value" == "1" ]
			then
				echo Option 1 is checked
			else
				echo Option 1 is unchecked
			fi
			;;
		txt1)
			echo Text entered: $value
			;;
		okay)
			flag=1
			echo User clicked Ok pushbutton
			;;
		cancel)
			flag=1
			echo User clicked Cancel pushbutton
			;;
	esac
done < <(

dialogbox <<EODEMO
add checkbox "&Option 1" cb1
add textbox "&Text field" txt1 "text to edit"
add groupbox horizontal
add stretch
add pushbutton O&k okay apply exit
add pushbutton &Cancel cancel exit
end groupbox
set okay default
set cb1 focus
EODEMO

)

if [ "$flag" == "0" ]
then
	echo User closed the window
fi
```
See full version of the script [here](./demos/demo2).

>Note: Values of the checkbox and the textbox controls are reported only on okay button click. The `flag` variable is used to distinguish event when the window was closed using the Window Manager's controls.

######Case 3: User input requires the dialog box to be modified (pipes)
Scenario: the script needs to modify the dialog box upon user's request. The user enters data and accepts the dialog. Alternatively the user might request some action the dialog's design depends on. The frequent case is the Apply button on which click the script has to apply the data entered by the user and update the dialog. Rejection of the dialog means cancellation of the process.

The application is used to build a dialog box with a pushbutton with `apply` option set. The latter is for user to apply values entered. For this task bidirectional communication with the `dialogbox` application is required. This case reviews pipes usage. The `bash` shell supports `coproc` command which runs a process in background with its standard input and output redirected to the pipes.

>Note: the `bash` shell supports only one co-process. Its PID is stored in `COPROC_PID` variable.

The bidirectional communication is realized by reading from the file descriptor the application writes to and writting to the file descriptor the application reads from. These file descriptors are provided by the shell as `COPROC[0]` and `COPROC[1]` respectively. For better reading below example script stores these descriptors in variables named `INPUTFD` (input for the script) and `OUTPUTFD` (output from the script) respectively. So, at any point the script can output commands to the `dialogbox` application by redirecting them to `&$OUTPUTFD` and can read the output of the application by reading from `&$INPUTFD`.

>Tip: if the script doesn't use its standard input/output they can be redirected to the pipes. This will simplify the script as there will be no redirections for each input/output command. Each output will go to the application and each input will be made from the application. Tools used within the script might output something to the pipe but it is unlikely it will be understood by the `dialogbox` application. The following two commands will redirect standard input and output to the pipes:<br>
>	`exec <&${COPROC[0]}`<br>
>	`exec >&${COPROC[1]}`

```shell
coproc dialogbox
INPUTFD=${COPROC[0]}
OUTPUTFD=${COPROC[1]}

cat >&$OUTPUTFD <<EODEMO
add groupbox horizontal
add checkbox "&Option 1" cb1
add pushbutton "&Disable option 1" dsbl
add groupbox horizontal
add stretch
add pushbutton O&k okay apply exit
add pushbutton &Cancel cancel exit
end groupbox
set okay default
set cb1 focus
EODEMO


flag=0
cbflag=0

while IFS=$'=' read key value
do
	case $key in
		dsbl)
			if [ "$cbflag" == "0" ]
			then
				echo disable cb1 >&$OUTPUTFD
				echo set dsbl title \"\&Enable option 1\" >&$OUTPUTFD
				cbflag=1
			else
				echo enable cb1 >&$OUTPUTFD
				echo set dsbl title \"\&Disable option 1\" >&$OUTPUTFD
				cbflag=0
			fi
			;;
		cb1)
		# Note: disabled widgets are not reported
			if [ "$value" == "1" ]
			then
				echo Option 1 is checked
			else
				echo Option 1 is unchecked
			fi
			;;
		okay)
			flag=1
			echo User clicked Ok pushbutton
			;;
		cancel)
			flag=1
			echo User clicked Cancel pushbutton
			;;
	esac
done <&$INPUTFD

if [ "$flag" == "0" ]
then
	echo User closed the window
fi
```
See full version of the script [here](./demos/demo3).

>Tip: some characters (e.g. `&`, `"`, `\`) when used within commands might need to be escaped to prevent their interpretation by the shell. See `echo` command in the example above.

As in previous case the loop ends if the co-process closes the writing end of the pipe that happens when the application terminates.

>Tip: if for a reason execution of the script breaks from the loop it can check whether the dialog was closed by the user with the below command:<br>
> `kill -0 $COPROC_PID &>/dev/null || exit`<br>
> It exits the script if the co-process no longer exists.

The `cbflag` variable is used to track the state of the checkbox. The latter is disabled/enabled by commands sent to the `dialogbox` application in response to `dsbl` pushbutton click. This demonstrates the dialog box modification.

######Case 4: User input requires the dialog box to be modified (FIFOs)
The scenario is absolutely the same as in the previous case. This case reviews usage of FIFOs (named pipes) for bidirectional communication with the `dialogbox` application.

Using FIFOs can be preferred in situations when the shell doesn't support a co-process, another co-process is already run, design of the script requires different processes to communicate with the `dialogbox` process, etc.

The drawbacks of such an approach are need in a writable filesystem and a cleanup code (to remove the FIFOs).

In the example below bidirectional communication is realized by writing to the FIFO the `dialogbox` application reads from and reading from the FIFO it writes to. The script removes files with names of FIFOs to avoid a situation when a regular files with the same names exist, creates the FIFOs and runs the `dialogbox` process in background with its standard input and output redirected to the FIFOs. Next the exit trap is set. The latter terminates the `dialogbox` process and removes the FIFOs from the file system. This cleanup code assures clean termination of the script not only at `exit` command but even if it is killed by a signal.

>Tip: as in the previous case both standard input and output of the script can be redirected to the FIFOs by the following commands:<br>
>`exec >"$FIFOOUT"`<br>
>`exec <"$FIFOIN"`<br>
>This gives advantage that the FIFOs can already be removed from the file system and this eliminates the need in the cleanup code (the exit trap).

```shell
FIFOIN="./pipe-in"
FIFOOUT="./pipe-out"

rm -rf "$FIFOIN" "$FIFOOUT"	# sanity removal

mkfifo "$FIFOIN" "$FIFOOUT"

dialogbox <"$FIFOOUT" >"$FIFOIN" &
DBPID=$!			# PID of the dialogbox

trap "kill $DBPID &>/dev/null; wait $DBPID; rm -rf \"$FIFOIN\" \"$FIFOOUT\"" EXIT

cat >"$FIFOOUT" <<EODEMO
add groupbox horizontal
add checkbox "&Option 1" cb1
add pushbutton "&Disable option 1" dsbl
add groupbox horizontal
add stretch
add pushbutton O&k okay apply exit
add pushbutton &Cancel cancel exit
end groupbox
set okay default
set cb1 focus
EODEMO


flag=0
cbflag=0

while IFS=$'=' read key value
do
	case $key in
		dsbl)
			if [ "$cbflag" == "0" ]
			then
				echo disable cb1 >"$FIFOOUT"
				echo set dsbl title \"\&Enable option 1\" >"$FIFOOUT"
				cbflag=1
			else
				echo enable cb1 >"$FIFOOUT"
				echo set dsbl title \"\&Disable option 1\" >"$FIFOOUT"
				cbflag=0
			fi
			;;
		cb1)
		# Note: disabled widgets are not reported
			if [ "$value" == "1" ]
			then
				echo Option 1 is checked
			else
				echo Option 1 is unchecked
			fi
			;;
		okay)
			flag=1
			echo User clicked Ok pushbutton
			;;
		cancel)
			flag=1
			echo User clicked Cancel pushbutton
			;;
	esac
done <"$FIFOIN"

if [ "$flag" == "0" ]
then
	echo User closed the window
fi
```
See full version of the script [here](./demos/demo4).

######Case 5: Manage a background process with GUI frontend
Scenario: the script runs a long-run process and presents the user a frontend dialog with information about the process, a spinner, etc. and with a Cancel pushbutton. The script has to terminate the background process immediately once the user cancels it (rejects the dialog).

The example script uses the shell's child processes monitoring option which enables the SIGCHLD signal and sets the trap for this signal. As there is need to modify the dialog box it uses the `coproc` command for bidirectional communication with the `dialogbox` application (see [case 3](#case-3-user-input-requires-the-dialog-box-to-be-modified-pipes) above for details).

The trap for the SIGCHLD signal checks if it was caused by the `dialogbox` process termination. If so it kills the background job and terminates the script with exit status of `E_CANCEL` value.

Next, the script draws, say, "welcome" dialog with options Next and Cancel and waits in the loop for the Next button clicked by the user. Any other action causes the `dialogbox` process to terminate with SIGCHLD signal caught by the trap. The latter terminates the script.

Once the script continues it modifies the dialog with appropriate information and leaves only Cancel pushbutton. The long-run job is run in the background to return control to the shell to allow it to process signals. In the below example the `sleep 10&` command simulates the long-run process.

At this point the background job is waited for. If the user cancels the process (clicks the Cancel button, presses Esc key, etc.) the `dialogbox` process is terminated and the trap catches the signal which in turn terminates the long-run process and exits the script with exit status of `E_CANCEL` value.

But if the user did not cancel the process it finishes successfully and the script modifies the dialog appropriately, switches off the child processes monitoring and simply waits for the user to complete interaction with the dialog. The script completes with exit status of `E_SUCCESS`.

```shell
coproc dialogbox
INPUTFD=${COPROC[0]}
OUTPUTFD=${COPROC[1]}
DBPID=$COPROC_PID

# Exit status values:
E_SUCCESS=0
E_CANCEL=1

trap "if ! kill -0 $DBPID &>/dev/null; then kill %; echo The script cancelled by user; exit $E_CANCEL; fi" CHLD
set -o monitor	# Enable SIGCHLD

cat >&$OUTPUTFD <<EODEMO
add label "Click Next to start a long-run process" msg
add groupbox horizontal
add stretch
add pushbutton &Next okay
add pushbutton &Cancel cancel exit
end groupbox
set okay default
set okay focus
EODEMO

while IFS=$'=' read key value
do
	case $key in
		okay)
			echo User initiated the long-run process
			break
			;;
	esac
done <&$INPUTFD

cat >&$OUTPUTFD <<EODEMO
set msg title "The long-run process is in progress.<br>Please wait..."
remove okay
set cancel default
set cancel focus
EODEMO

sleep 10&
wait %
echo The long-run process completed

cat >&$OUTPUTFD <<EODEMO
set msg title "The long-run process completed!"
set cancel title &Ok
EODEMO

set +o monitor	# Disable SIGCHLD
wait $DBPID		# Wait the user to complete the dialog

echo The script completed successfuly

exit $E_SUCCESS
```
See full version of the script [here](./demos/demo5).

#####Examples
The [examples](./examples/) directory contains various fully functional applications which are the `bash` scripts but thanks to GUI provided by the `dialogbox` application offer user friendly look and feel. All of them have minimalistic design, minimum dependencies, are desktop agnostic and offer solutions to replace "heavy" desktop applets.

These applications are independent projects but all are collected here as they are good examples of the `dialogbox` application usage and demonstrate what it was designed for.

More applications to come.

#####Bug Reporting
You can send `dialogbox` bug reports and/or any compatibility issues directly to the author [martynets@volia.ua](mailto:martynets@volia.ua).

You can also use the online bug tracking system in the GitHub `dialogbox` project to submit new problem reports or search for existing ones:

  https://github.com/martynets/dialogbox/issues

#####Change Log
0.8 Initial development, non-released version.

#####License
Copyright (C) 2015 Andriy Martynets [martynets@volia.ua](mailto:martynets@volia.ua)<br>
This file is part of `dialogbox`.

`dialogbox` is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

`dialogbox` is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
`dialogbox`.  If not, see <http://www.gnu.org/licenses/>.
