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

I've made the process super easy for Windows, as long as you have Visual Studio 2008. Just load up the solution file and compile away.
With Visual Studio 2015-2019 things get a bit more involved and it all depends on how far you're willing to go to build on modern VS.

You can go one of two ways with modern VS:
1. Install Visual Studio 2008, 2010, and your modern VS of choice (2015-2019). Load the solution file into your modern VS and target the toolset for VS2008 then compile away.
2. Use just your modern Visual Studio and change the additional include and lib directories to point to the VS2015 equivalent folders in the dev-libs directory. The only downside to this is that it breaks Net-Melee functionality.

For other platforms you'll have to gather all of the necessary dependencies and hope for the best.

## Contributors

Me (Serosis), SlightlyIntelligentMonkey, and Ala-lala

The main menu music for the MegaMod is brought to you by Saibuster A.K.A. Itamar.Levy: https://soundcloud.com/itamar-levy-1/star-control-hyperdrive-remix, Mark Vera A.K.A. Jouni Airaksinen: https://www.youtube.com/watch?v=rsSc7x-p4zw, and Rush AX: http://star-control.com/fan/music.php.

And the default Super Melee menu music is by Flashy of Infinitum.

## License

The code in this repository is under GNU General Public License, version 2 http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The content in this repository is under Creative Commons Attribution-NonCommercial-ShareAlike, version 2.5 https://creativecommons.org/licenses/by-nc-sa/2.5/
