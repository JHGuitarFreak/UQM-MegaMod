# The Ur-Quan Masters MegaMod
A fork of The Ur-Quan Masters that continues the HD mod with a veritable smorgasbord of extra features.

## Changes from the original

A full list of changes and features can be found on the [Main site](http://megamod.serosis.net/Features).

And the current changelog can be found in the repository: [Changelog](https://github.com/Serosis/UQM-MegaMod/blob/master/MegaMod%20Changelog.txt)

## Motivation

This project exists out of my love for Star Control II and The Ur-Quan Masters project. Fiddling with the code as a hobby to try and expand my skill as a programmer.

## Windows Installation

Open up the installer, pick your optional content, wait for all of it to download and install, then play!

## MacOS X Installation

Mount the .dmg file and copy the app to your Applications folder.

## Building Yourself

### Windows

#### Visual Studio 
I've made the process super easy for Windows, as long as you have Visual Studio 2008 or Visual Studios 2015-2019. Just load up the solution file and compile away.
For Visual Studio 2008 the solution file is under `build/msvs2008` for Visual Studios 2015-2019 the solution file is under `build/msvs2019`

#### MSYS2

Make sure you've installed all the necessary packes by executing these two commands in the MSYS2 bash:

	pacman -Syuu

then

	pacman -S make pkg-config mingw-w64-i686-gcc mingw-w64-i686-libogg \
		mingw-w64-i686-libpng mingw-w64-i686-libsystre \
		mingw-w64-i686-libvorbis mingw-w64-i686-SDL2 mingw-w64-i686-zlib

Start a MSYS2 MinGW 32-bit bash, `cd` to the UQM-MegaMod directory, then execute this command: 

	./build.sh uqm 

When executing this command for the first time you'll come to a configuration screen where you can select a few developer-centric options.
Just hit enter and UQM will start building. It'll take awhile and you'll see a few scary warnings but everything should build fine.

Missing .dll can be found in the MSYS2 installation folder under `mingw32\bin\`

### Other Platforms
You'll have to gather all of the necessary dependencies and hope for the best.

#### Tips

For all platforms when building from commandline you can use the command `-j#` to invoke multi-threaded performance to dramaticly speed up build time.  
Example: If you're running on a Ryzen 7 2700x you can use the command like so `./build.sh -j16 uqm` to take advantage of all your threads.

## Fixes

### Windows

#### Image Cut-off in fullscreen

The most common contributor to this is the Windows built in DPI settings that scales UI for larger resolutions.
You can fix this one of two ways, by setting the UI scaling in Windows to 100% or overriding the scaling on UQM itself.

For overriding on UQM itself:  
Right click on the EXE -> click `Properties` -> click over to the `Compatibility` tab -> click `Change high DPI settings` -> click `Override high DPI scaling behavior` -> set `Scaling performed by:` to `Application`  
It should look like this:  
![image](https://user-images.githubusercontent.com/4404965/80047996-bb0e7f00-84c3-11ea-8914-85509e2fb623.png)

## Controllers

When using a DualShock 4 controller *DO NOT* use DS4Windows. UQM and hence, MegaMod, support the DS4 without extra software.
The new controller layout option only works if you *don't* use DS4Windows.

Have a lovely day!


## Contributors

Me (Serosis), SlightlyIntelligentMonkey, Volasaurus, Ala-lala, and Kruzenshtern

The main menu music for the MegaMod is brought to you by Saibuster A.K.A. Itamar.Levy: http://star-control.com/fan/music/Various/saibuster-hyprespace.mp3, Mark Vera A.K.A. Jouni Airaksinen: https://www.youtube.com/watch?v=rsSc7x-p4zw, and Rush AX: http://star-control.com/fan/music/Rush/HSpace%20Rush%20MIX.mp3

And the default Super Melee menu music is by Flashy of Infinitum.

## License

The code in this repository is under GNU General Public License, version 2 http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The content in this repository is under Creative Commons Attribution-NonCommercial-ShareAlike, version 2.5 https://creativecommons.org/licenses/by-nc-sa/2.5/
