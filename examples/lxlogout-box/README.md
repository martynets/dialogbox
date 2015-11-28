Copyright (C) 2015 Andriy Martynets [martynets@volia.ua](mailto:martynets@volia.ua)<br>
See the end of the file for license conditions.

-------------------------------------------------------------------------------

#####Introduction
`lxsession-logout` is the default quit manager for `LXSession`. It silently depends on a list of packages which ends up with `PolicyKit` and `ConsoleKit`.

For system administrators who tend to minimalistic design or are concerned on security the `lxlogout-box` script might be an option.

It fully emulates look and feel of the original `lxsession-logout` application and depending on your current theme will look similar to the following:

![](../../images/lxlogout-box.png)

It is not designed specially for LXDE but uses images from that package. That is the reason for the first two letters in its name. Generally it can be used for any environment.

#####Software requirements
`lxlogout-box` script functionality is based on the `dialogbox` application which can be downloaded from [the dialogbox repository](https://github.com/martynets/dialogbox).
Also the script uses tools from `sysvinit-core` and `pm-utils` packages and expects they are allowed for ordinar users via the `sudo`. If not please copy the following to a file with convenient name (say `powermanagement`) and save it to `/etc/sudoers.d` directory:

```
# This allows any user to reboot, poweroff, suspend and hybernate the system


Cmnd_Alias	POWER = /sbin/shutdown, /sbin/halt, /sbin/poweroff, /sbin/reboot, /usr/sbin/pm-suspend, /usr/sbin/pm-hibernate

ALL		ALL = NOPASSWD : POWER
```
Optionally the script uses [xdg-bash-functions package](https://github.com/martynets/xdg-bash-functions), if the last one present, for operations with your current icon theme. This package can be downloaded and installed by the `installer` during the installation process (see below).

#####Downloading
This script is shipped as an example for [the dialogbox application](https://github.com/martynets/dialogbox/) and can be downloaded from its repository.

#####Installation
This script doesn't require a specific installation and can be called from any location. To be used alone it can be copied to `/usr/bin` directory. To be used as the replacement for `lxsession-logout` a symlink to it must be created named `lxde-logout`. It is advised to save backup copy of the original `lxde-logout` script.
All these installation tasks are done by the `installer` script present in the current directory. It provides user-friendly GUI interface and does the following:
- renames `lxde-logout` script to `lxde-logout-original`
- copies `lxlogout-box` script to `/usr/bin` directory
- creates symlink to it named `lxde-logout`
- if user confirms, the `installer` downloads and installs [xdg-bash-functions package](https://github.com/martynets/xdg-bash-functions/) mentioned above.

All these tasks require root privilages. The installer tries to re-run itself with prompt for root password or asks user to run it with root account.
> Note: the `installer` script must be run from the source tree as it uses some shared files. It is designed for Debian derivatives and needs minor adjustments for other systems.

To uninstall the `lxlogout-box` script the above tasks must be undone manually.

#####Usage
The `lxlogout-box` script doesn't accept any command line arguments. It can be called directly or your desktop solution must be configured in its specific way to run it as the quit manager. For the `LXDE` the configuration is done during the installation described above.
The `lxlogout-box` script does some heuristic to detect commands for screen lock, logout and user switch. This heuristic is similar to what the `lxsession-logout` application does. If this doesn't suit your particular environment the beginning of the script may be revised (it is commented in appropriate way).

#####Bug Reporting
You can send `lxlogout-box` bug reports and/or any compatibility issues directly to the author [martynets@volia.ua](mailto:martynets@volia.ua).

You can also use the online bug tracking system in the GitHub `dialogbox` project to submit new problem reports or search for existing ones:

  https://github.com/martynets/dialogbox/issues

#####Change Log
1.0    Initial release

#####License
Copyright (C) 2015 Andriy Martynets [martynets@volia.ua](mailto:martynets@volia.ua)<br>
This file is part of `lxlogout-box`.

`lxlogout-box` is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

`lxlogout-box` is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
`lxlogout-box`.  If not, see <http://www.gnu.org/licenses/>.
