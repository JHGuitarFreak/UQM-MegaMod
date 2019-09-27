# MegaMod ReadMe

Explanation of new Setup Menu options:

# Graphics Options
	
	Graphics - Toggles between original and HD graphics.

	Resolution - Scales the graphics of Original and HD graphics to the selected resolution.
								Disclaimer: When switching to HD graphics for the first time make sure to choose 'Default' resolution.

# PC/3DO Options

	Melee Zoom - For Android the Melee Zoom option has been 'enhanced' to allow the user to choose which melee scaler to improve game speed. The choices are between Step, Nearest, Bilinear, and Trilinear.
								The lowest option is a step zoom and will be the fastest on all devices. The 'Nearest' scaler will be the fastest for Smooth Scaling melee. Your mileage may vary depending on your device.

	IP Transitions - This option allows the user to switch between Stepped zooming and Crossfade zooming in interplanetary view. Stepped is the method used in the origina PC version of SC2.
										While Crossfade is what is used in the 3DO version of SC2. There is no performance difference between the two although the Stepped method has instantaneous transition.

	Lander Hold Size - This option lets you choose between the PC and 3DO version of the Lander's basic cargo capacity. For PC it was 64 units. For 3DO it was 50 units.

# Sound Options

	# Music Configuration - This option takes you to the Music Configuration menu

		Volasaurus' Remixes - Toggles on or off remixes made by Volasaurus. Only applicable if you have the package installed.

		SOI Space Music - This option can turn on or off music that plays when inside the Sphere of Influence (while in a solar system) of an alien race.
												Music included composed by Volasaurus.

# Advanced Options

	Difficulty - Toggles between Normal, Easy, and Hard difficulties. 
								The differences are as follows:

									# Hard Mode
										- All Spathi escorts flee when they slave shield themselves
										- Starting crew is 31 to account for the epilogue story
										- Amount of Thraddash to defeat is 35 to become allied
										- Crew Cost is minimally reduced when the Shofixti are revived
										- Slylandro Probes dodge projectiles
										- Mineral worth for Exotics is brought down to 16
										- Slylandro Probes will orbit Rainbow Worlds, their numbers increase if left unchecked and will eventually vanish once you complete their quest
										- New game will have a random custom seed if the set, loaded, or commandline seed isn't the default.
										- Rainbow World credit value cut in half
										- Weapon slots 2 & 3 take double energy
										- Auto tracking costs double energy
										- Evil Ones are now more dangerous, they can move and take 5 shots to can them. They are also worth 5 bio-units.
										- Melnorme tech catalog spread out between the 9 Supergiants (No tech upgrades in Hyperspace)
										- Melnorme fuel cost upped to 10 credits per unit
										- Randomize Quasi Portal exits even without a custom seed (Portal exits maintain their position and are not randomized every time)
										- Reduced viability of the Druuge/Melnorme fuel exploit
										- If player kills Tanaka they do not get a second chance with Katana
										- Portal Spawner costs 20 whole units of fuel
										- Selling more than 10 crew to the Druuge guarantees a price hike in crew cost and the immediate disapproval of the Commander
										- Can only build two of any ship
										- Tanaka's Scout only has the glory device disabled

									# Easy Mode
										- Double mineral worth
										- Fwiffo starts with a full crew compliment
										- Amount of Thraddash to defeat is 15 to become allied
										- Tune down the battle difficulty to Weak Cyborg
										- Start with two landers
										- Rainbow World credit value doubled
										- Add a couple more fusion thrusters and turning jets for new game
										- Keep the Emergency Escape Warp Unit enabled after Chmmr Bomb installation
										- Portal Spawner costs 5 whole units of fuel
										- Player starts with 43.38 units of fuel (The difference between a full tank and the amount of fuel it takes to travel from Vela to Sol)
										- Player starts with an extra 2500 R.U.
										- Limit to the amount of crew to sell to the Druuge before crew cost hike is 200
										- Commander will tell you about the Melnorme and ZFP earlier than usual

	Extended Lore - Enables extra lore provided by 0xDEC0DE's UQM-Extended mod plus a few suggested by Tarp from the Alliance Discord server.
									The additions are as follows:

										# Extended Lore
											- No longer possible to encounter random Ur-Quan ships in the Zoq-Fot-Pik home system, like their dialogs say (Bugzilla Bug #759)
											- Add a "dead" slave-shielded world and starbase @ Beta Ophiuchi I (047.9:887.5)
											- Destroyed Ur-Quan(*) / Kohr-Ah(+) starbases
												+ Beta Reticuli (702.0 : 529.1)
												* Metis (570.8 : 460.4)
												* Gamma Cancri (500.6 : 501.1)
												* Alpha Lentilis (462.5 : 600.0)
												+ Theta Chamaeleonis (514.5 : 695.8)
												+ Antares (647.9 : 754.1)
											- The Syreen will have a Sphere of Influence at the completion of their Vault Quest
											- The Chmmr will have a Sphere of Influence once you help them complete their Process
											- Defunct Mother-Ark @ Delta Virginis IV
											- Stripped Precursor Starbase @ Alpha Centauri
											- Ruins and lore on Algol IV to match with the lore as told by the Melnorme
											- Add a Spathi erected monument to the peaceful negotiatons of colonization between them and the Mmrnmhrm on Beta Herculis II
											- The Thraddash will survive the Ilwrath war if you are allied with them. And they will let you take the AquaHelix peacefully

	Nomad Mode - Enables a survival version of the gameplay where you do not have access to the Earth Starbase.
								The changes are as follows:

									# Nomad Mode
										- New message report for the Earth Starbase when you try to dock
										- Player will no longer "pick up" the base on Luna
										- Emergency Warp Unit activated without needing to visit the Starbase
										- Add a starbase around Kyabetsu that will give the player a max of two Scouts at a time after their quest is completed
										- Give player two extra jets and thrusters so as to not be completely outmatched in speed
										- Make Probes spawn at a reasonable rate so as to not annoy the player
										- When allied, Spathi ships can join your fleet when asked for a max of 3 ships in your fleet at a time.
										- Player starts with 20 more units of fuel

	Custom Seed - A text input option that allows you to change the universe seed. Changing the seed generates new star patterns, planet order, planet type, and number of planets where applicable,
								moon type, number of moons, and Quasispace portal exits. Planets and moons included in 'spoken' lore do not change order but can be different types that are similar to their original type.

	Visual Configuration - This option takes you to the Visual Configuration screen

		# Visual Configuration

			Scale Planets - Enables you to scale planets up proportionally for HD. Turning this off in HD makes planets extremely tiny.

			Date Format - Allows you to choose between popular date formats for the date display in-game.

			Custom Border - Enables a custom UI overlay based on Clay's "sc2redo" mock-up.

			Whole Fuel Value - When enabled shows you the full fuel value in decimal format

			Fuel Range - Enables the 'point of no return' fuel circle on the Starmap.
