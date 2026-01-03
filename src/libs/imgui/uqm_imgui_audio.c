//Copyright Paul Reiche, Fred Ford. 1992-2002
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "uqm_imgui.h"
#include "libs/sound/sound.h"

void draw_audio_menu (void)
{
	const char *sound_drivers[] = {"None", "MixSDL", "OpenAL"};
	const char *sound_qualities[] = {"Low", "Medium", "High"};
	const char *alien_ambience[] = {"Disabled", "No Spoilers", "Spoilers"};
	const char *music_resume[] = {"Disabled", "5 Minutes", "Indefinite"};

	ImGui_ColumnsEx (DISPLAY_BOOL, "AudioColumns", false);

	// Sound Options
	{
		ImGui_SeparatorText ("Sound Options");

		{
			int volume = musicVolumeScale * 100;

			ImGui_Text ("Music Volume:");
			if (ImGui_SliderInt ("##MusicVolume", &volume, 0, 100))
			{
				musicVolumeScale = volume / 100.0f;
				SetMusicVolume (musicVolume);

				res_PutInteger ("config.musicvol", volume);
				config_changed = true;
			}
		}

		{
			int volume = sfxVolumeScale * 100;

			ImGui_Text ("SFX Volume:");
			if (ImGui_SliderInt ("##SFXVolume", &volume, 0, 100))
			{
				sfxVolumeScale = volume / 100.0f;
				SetSFXVolume (sfxVolumeScale);

				res_PutInteger ("config.sfxvol", volume);
				config_changed = true;
			}
		}

		{
			int volume = speechVolumeScale * 100;

			ImGui_Text ("Speech Volume:");
			if (ImGui_SliderInt ("##SpeechVolume", &volume, 0, 100))
			{
				speechVolumeScale = volume / 100.0f;
				SetSpeechVolume (speechVolumeScale);

				res_PutInteger ("config.speechvol", volume);
				config_changed = true;
			}
		}

		Spacer ();

		ImGui_BeginDisabled (true);
		{

			UQM_ImGui_CheckBox ("Positional Audio", &optStereoSFX,
					"config.positionalsfx");
		}

		Spacer ();

		{
			ImGui_BeginDisabled (true);

			int driver;

			switch (snddriver)
			{
				case audio_DRIVER_MIXSDL:
					driver = 1;
					break;
				case audio_DRIVER_OPENAL:
					driver = 2;
					break;
				case audio_DRIVER_NOSOUND:
				default:
					driver = 0;
			}

			ImGui_Text ("Sound Driver:");
			if (ImGui_ComboChar ("##SoundDriver", &driver,
					sound_drivers, 3))
			{
				// Add switching code here
			}

			ImGui_EndDisabled ();
		}

		{
			int quality;

			if (soundflags & audio_QUALITY_HIGH)
				quality = OPTVAL_HIGH;
			else if (soundflags & audio_QUALITY_LOW)
				quality = OPTVAL_LOW;
			else
				quality = OPTVAL_MEDIUM;

			ImGui_Text ("Sound Quality:");
			if (ImGui_ComboChar ("##SoundQuality", &quality,
					sound_qualities, 3))
			{
				// Add switching code here
			}
		}

		ImGui_TextWrappedColored (IV4_RED_COLOR,
			"WARNING! All of these disabled options can not be changed "
			"in the GUI at this time. To change them you must use the "
			"Setup Menu.");

		ImGui_EndDisabled ();
	}

	ImGui_NewLine ();

	// Music Options
	{
		ImGui_SeparatorText ("Music Options");

		ImGui_BeginDisabled (true);

		ImGui_Checkbox ("3DO Remixes", (bool *)&opt3doMusic);
		ImGui_Checkbox ("Precursor's Remixes", (bool *)&optRemixMusic);
		ImGui_Checkbox ("Volasaurus' Remixes", (bool *)&optVolasMusic);

		Spacer ();

		{
			ImGui_Text ("Interplanetary Alien Ambience:");
			if (ImGui_ComboChar ("##InterplanetaryAlienAmbience",
					&optSpaceMusic, alien_ambience, 3))
			{
				// Add switching code here
			}
		}

		ImGui_TextWrappedColored (IV4_RED_COLOR,
			"WARNING! All of these disabled options can not be changed "
			"in the GUI at this time. To change them you must use the "
			"Setup Menu.");

		ImGui_EndDisabled ();

		Spacer ();

		UQM_ImGui_CheckBox ("Menu Music", &optMainMenuMusic, "mm.mainMenuMusic");

		Spacer ();

		{
			ImGui_Text ("Music Resume:");
			if (ImGui_ComboChar ("##MusicResume", &optMusicResume,
					music_resume, 3))
			{
				res_PutInteger ("mm.musicResume", optMusicResume);
				mmcfg_changed = true;
			}
		}
	}
}