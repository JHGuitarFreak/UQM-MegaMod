
![title](https://github.com/JHGuitarFreak/UQM-MegaMod/assets/104330805/d6f09a02-5eec-4bb7-81d6-4e5e049fd856)
___

<h4 align="center">A fork of The Ur-Quan Masters + HD-mod that remasters the HD graphics with a veritable smorgasbord of extra features, options, QoL improvements, and much more...</h4>

<p align="center">

<a href="https://sourceforge.net/projects/uqm-mods/files/MegaMod/0.8.3/">
<img alt="SourceForge Downloads (folder)" src="https://img.shields.io/sourceforge/dt/uqm-mods/MegaMod"></a>
<a href="https://github.com/JHGuitarFreak/UQM-MegaMod/pulls">
	<img src="https://img.shields.io/github/issues-pr/JHGuitarFreak/UQM-MegaMod" alt="Pull Requests Badge"/></a>	
<a href="https://github.com/JHGuitarFreak/UQM-MegaMod/issues">
	<img src="https://img.shields.io/github/issues/JHGuitarFreak/UQM-MegaMod" alt="Issues Badge"/></a>	
<a href="https://github.com/JHGuitarFreak/UQM-MegaMod/graphs/contributors">
	<img alt="GitHub contributors" src="https://img.shields.io/github/contributors/JHGuitarFreak/UQM-MegaMod?color=2b9348"></a>	
<a href="https://github.com/JHGuitarFreak/UQM-MegaMod/blob/master/LICENSE">
	<img src="https://img.shields.io/github/license/JHGuitarFreak/UQM-MegaMod?color=2b9348" alt="License Badge"/></a>
	
</br>

<a href="https://sourceforge.net/projects/uqm-mods/files/MegaMod/0.8.3/">
	<img src="https://img.shields.io/github/v/release/JHGuitarFreak/UQM-MegaMod?label=Latest%20release&style=social" alt="Latest release"/></a>	
<a href="https://GitHub.com/JHGuitarFreak/UQM-MegaMod/commit/">
	<img src="https://img.shields.io/github/commits-since/JHGuitarFreak/UQM-MegaMod/0.8.3.svg?style=social" alt="GitHub commits"/></a>	
<a href="https://github.com/JHGuitarFreak/UQM-MegaMod/stargazers">
	<img src="https://img.shields.io/github/stars/JHGuitarFreak/UQM-MegaMod" alt="Stars Badge"/></a>	
<a href="https://github.com/JHGuitarFreak/UQM-MegaMod/network/members">
	<img src="https://img.shields.io/github/forks/JHGuitarFreak/UQM-MegaMod" alt="Forks Badge"/></a>	
<a href="https://github.com/aregtech/areg-sdk/watchers">
	<img src="https://img.shields.io/github/watchers/aregtech/areg-sdk?style=social" alt="Watchers"/></a>
	
</p>
<p align="center">

<a href="https://uqm-mods.sourceforge.net/Discord">
	<img src="https://img.shields.io/badge/discord-7289da.svg?style=for-the-badge&logo=discord" alt="discord"></a>

</p>

___

## Changes from the original

The changes are too numerous to list here so I've provided a couple of links to help out in this regard...

A full list of changes and features for the current release can be found in the [README](doc/release/MegaMod-README.txt).

And the full changelog can be found in the root of this repository: [Changelog](MegaMod%20Changelog.txt)

## Install
<p align="center">

<a href="https://sourceforge.net/projects/uqm-mods/files/MegaMod/0.8.3/mm-0.8.3-win32.exe/download">
<img height="38" alt="SourceForge Downloads (folder)" src="https://img.shields.io/sourceforge/dt/uqm-mods/MegaMod%2F0.8.3%2Fmm-0.8.3-win32.exe?style=flat-square&logo=windows&logoSize=auto&label=Windows"></a>
<a href="https://sourceforge.net/projects/uqm-mods/files/MegaMod/0.8.3/mm-0.8.3-linux.deb/download">
<img height="38" alt="SourceForge Downloads (folder)" src="https://img.shields.io/sourceforge/dt/uqm-mods/MegaMod%2F0.8.3%2Fmm-0.8.3-linux.deb?style=flat-square&logo=debian&logoSize=auto&label=Linux%20(Debian)"></a>
<a href="https://sourceforge.net/projects/uqm-mods/files/MegaMod/0.8.3/mm-0.8.3-macos.dmg/download">
<img height="38" alt="SourceForge Downloads (folder)" src="https://img.shields.io/sourceforge/dt/uqm-mods/MegaMod%2F0.8.3%2Fmm-0.8.3-macos.dmg?style=flat-square&logo=apple&logoSize=auto&label=macOS%20(10.13)"></a>

<br>

<a href="https://sourceforge.net/projects/uqm-mods/files/MegaMod/0.8.3/mm-0.8.3-android-SDL1.apk/download">
<img height="38" alt="Android Download" src="https://img.shields.io/sourceforge/dt/uqm-mods/MegaMod%2F0.8.3%2Fmm-0.8.3-android-SDL1.apk?style=flat-square&logo=android&logoSize=auto&label=Android"></a>
<a href="https://flathub.org/apps/net.sourceforge.uqm_mods.UQM-MegaMod">
<img height="38" alt="Flathub Download" src="https://img.shields.io/flathub/downloads/net.sourceforge.uqm_mods.UQM-MegaMod?style=for-the-badge&logo=flathub&logoSize=auto&label=FlatHub"></a>

</p>

## Compiling

### Requirements

* Environment:
	* Windows 10+ ([MSYS2/MinGW](https://www.msys2.org/) or [Visual Studio](https://visualstudio.microsoft.com/vs/community/) recommended)
	* macOS 10.13+ ([brew](https://brew.sh/) recommended for personal builds)
	* Any modern Linux distribution
* Hard Dependencies
	* [SDL2](https://www.libsdl.org/)
	* [libPNG](http://www.libpng.org/pub/png/libpng.html)
	* [libOGG](https://xiph.org/ogg/)
	* [libVorbis](https://xiph.org/vorbis)
	* [zlib](https://www.zlib.net/)
	* [UQM-MegaMod-Content](https://github.com/JHGuitarFreak/UQM-MegaMod-Content/)
* Optional Dependencies
	* [SDL1](https://github.com/libsdl-org/SDL-1.2) (For systems that do not support SDL2)


> [!IMPORTANT]  
> After downloading or cloning this repository make sure you install the UQM-MegaMod-Content first before trying to run the compiled binary  
 
<details>
<summary>UQM-MegaMod Content</summary>

This process assumes you've downloaded or cloned this repository already.

Download or clone the [UQM-MegaMod-Content](https://github.com/JHGuitarFreak/UQM-MegaMod-Content/) repository and copy *all* the files within the content repository into the `UQM-MegaMod/content` folder of your downloaded or cloned UQM-MegaMod repository.

It should look like this: 

![Content Repo Preview](https://github.com/JHGuitarFreak/UQM-MegaMod/assets/104330805/9da1969c-a514-45fd-8826-842a1f256fd5)

</details>


<details>
<summary>Windows 10+</summary>

#### Visual Studio 

I've made this process super easy, as long as you have Visual Studio 2008 or Visual Studio 2015-2022.  
For Visual Studio 2008 the solution file is under `UQM-MegaMod/build/msvs2008` for Visual Studio 2015-2022 the solution file is under `build/msvs2019`  
Just load up the solution file and compile away.

Once the build is complete you'll either have a `UrQuanMasters.exe` or `UrQuanMastersDebug.exe` in the root directory that you can run directly.

If you get a message about missing .dll files they can be found in the `UQM-MegaMod/dev-lib/lib` directory.
Copy them to the root UQM-MegaMod directory.  
The .dll are as follows:

	libpng16.dll
	ogg.dll
	OpenAL32.dll
	SDL2.dll
	vorbis.dll
	vorbisfile.dll
	wrap_oal.dll
	zlib1.dll

#### MSYS2

Make sure you've installed all the necessary packages by executing these two commands in the MSYS2 bash:

	pacman -Syuu

then

	pacman -S make pkg-config mingw-w64-i686-gcc mingw-w64-i686-libogg \
		mingw-w64-i686-libpng mingw-w64-i686-libsystre \
		mingw-w64-i686-libvorbis mingw-w64-i686-SDL2 mingw-w64-i686-zlib

Start a MSYS2 MinGW 32-bit bash, `cd` to the UQM-MegaMod directory, then execute this command: 

	./build.sh uqm -j

When executing this command for the first time you'll come to a configuration screen where you can select a few developer-centric options.
Just hit enter and UQM will start building. It'll take awhile and you'll see a few scary warnings but everything should build fine.

Once the build is complete you'll either have a `UrQuanMasters.exe` or `UrQuanMastersDebug.exe` in the root directory that you can run directly.

If you get a message about missing .dll files they can be copied to the root directory via running the `msys2-depend.sh` bash script like so:

	./msys2-depend.sh

Note though that this script does not work for Visual Studio compiled binaries.

</details>

<details>
<summary>Linux</summary>

On Debian based distros it's fairly simple, just install the following packages:  

	sudo apt-get install build-essential libogg-dev libpng-dev \
			libsdl2-dev libvorbis-dev libz-dev

Then when those have finished installing you can either clone the repository or download the source tarball and extract it wherever you like, taking note of where it is.

`cd` to the UQM-MegaMod directory, then execute this command: 

	./build.sh uqm -j

When executing this command for the first time you'll come to a configuration screen where you can select a few developer-centric options.
Just hit enter and UQM will start building. It'll take awhile and you'll see a few scary warnings but everything should build fine.

Once the build is complete you'll either have a `UrQuanMasters` or `UrQuanMastersDebug` binary in the root directory that you can run directly.

</details>

<details>
<summary>macOS</summary>

Install Xcode from the App Store, and then when you run it the first time make sure to install "Additional components".  
You can then install brew from https://brew.sh and then use it to install your requirements from the Terminal:

	brew install libogg libpng libvorbis sdl2


Then when those have finished installing you can either clone the MegaMod repository or download the source tarball and extract it wherever you like, taking note of where it is.

`cd` to the UQM-MegaMod directory, then execute this command: 

	./build.sh uqm -j

When executing this command for the first time you'll come to a configuration screen where you can select a few developer-centric options.
Just hit enter and UQM will start building. It'll take awhile and you'll see a few scary warnings but everything should build fine.

Once the build is complete you'll either have a `UrQuanMasters` or `UrQuanMastersDebug` binary in the root directory that you can run directly.

</details>

___

> [!WARNING]  
> ### Controllers
> When using a DualShock 4 controller *DO NOT* use DS4Windows. UQM and hence, MegaMod, support the DS4 without extra software.
The new controller layout options only work properly if you *don't* use DS4Windows.

> [!CAUTION]
> ### Mod Compatibility
>
> MegaMod is not compatible with any other mods nor are they compatible with MegaMod.
MegaMod is completely independant from Core UQM, HD-Mod, HD-Remix, Crazy Mod, Balance Mod, Extended, or any other mods.
As such MegaMod has its own packages and add-ons.
>
> For example the HD-Mod add-on package `hires4x.zip` is not compatible with MegaMod as MegaMod has its own HD package currently named `mm-0.8.3-hd-content.uqm`.
>
> And as MegaMod is not compatible with any other mods, do not install it over any existing UQM installations.
>
> All MegaMod compatible add-ons and content can be found on the main website's Download page: https://uqm-mods.sourceforge.net/Download

___

## Contributors

Me (JHGuitarFreak), SlightlyIntelligentMonkey, Volasaurus, Ala-lala, Kruzenshtern, Taleden, and Jordan Lo.

The main menu music for the MegaMod is brought to you by Saibuster A.K.A. Itamar.Levy: http://star-control.com/fan/music/Various/saibuster-hyprespace.mp3, Mark Vera A.K.A. Jouni Airaksinen: https://www.youtube.com/watch?v=rsSc7x-p4zw, and Rush AX: http://star-control.com/fan/music/Rush/HSpace%20Rush%20MIX.mp3

And the default Super Melee menu music is by Flashira Nakirov.