#ifndef GENDEF_H
#define GENDEF_H

// JSD planets/generate.h no longer needed once GenerateFunctions is moved there.  It's where it belongs
// and the include of generate.h is causing circular definitions elsewhere.
// Added ARILOU_DEFINED = 0 (for location of quasi-portal and arilou zone of influence).
// Split RAINBOW_DEFINED into separate entries (like MELNORME0_DEFINED is) so they can be scattered.
// Added NUM_PLOTS for plot iteration.
// These are used in the plot randomizer.
//#include "planets/generate.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

//const GenerateFunctions *getGenerateFunctions (BYTE Index);

enum
{
	ARILOU_DEFINED = 0,
	SOL_DEFINED = 1,		// gensol.c, Super hardcoded, must be yellow dwarf
	SHOFIXTI_DEFINED,		// gensho.c, planet 1* randomly METAL and SELENIC
	MAIDENS_DEFINED,		// genvux.c, planet 1 [habitable] (redux)
	START_COLONY_DEFINED,	// gencol.c, planet 1 [habitable] (water)
	SPATHI_DEFINED,			// genspa.c, planet 1 [habitable] (water)
	ZOQFOT_DEFINED,			// genzoq.c, planet 1 [habitable] (redux)

	MELNORME0_DEFINED,
	MELNORME1_DEFINED,		// genmel.c, planet 3/rand [crystal]
	MELNORME2_DEFINED,
	MELNORME3_DEFINED,
	MELNORME4_DEFINED,
	MELNORME5_DEFINED,
	MELNORME6_DEFINED,
	MELNORME7_DEFINED,		// genmel.c, planet 4/rand [any rocky]
	MELNORME8_DEFINED,

	TALKING_PET_DEFINED,	// genpet.c, planet 1 [habitable] (telluric)
	CHMMR_DEFINED,			// genchmmr.c, planet 2/rand [crystal] (sapphire)
	SYREEN_DEFINED,			// gensyr.c, planet 1 [habitable] (water)
	BURVIXESE_DEFINED,		// genburv.c, planet 1 [habitable] (redux) A [rand]
	SLYLANDRO_DEFINED,		// gensly.c, planet 4/rand [gas giant]
	DRUUGE_DEFINED,			// gendru.c, planet 1 [desolate] (dust)
	BOMB_DEFINED,			// genutw.c, planet 6 [gas] B [rand] (carbide)
	AQUA_HELIX_DEFINED,		// genthrad.c planet 1 [habitable] (primordial)
	SUN_DEVICE_DEFINED,		// genmyc.c, planet 1 shattered
	TAALO_PROTECTOR_DEFINED,// genorz.c, planet 2 [gas] rand [crystal] (treas)
	SHIP_VAULT_DEFINED,		// genvault.c, planet 1 [lg rocky] A [rand] (green)
	URQUAN_WRECK_DEFINED,	// genwreck.c, planet 7 [sm rocky] (noble)
	VUX_BEAST_DEFINED,		// genvux.c, planet 1 [habitable] (water)
	SAMATRA_DEFINED,		// gensam.c, planet 5/rand [random]
	ZOQ_SCOUT_DEFINED,
	MYCON_DEFINED,			// genmyc.c, planet 1 shattered
	EGG_CASE0_DEFINED,		// genmyc.c, planet 1 shattered
	EGG_CASE1_DEFINED,		// genmyc.c, planet 1 shattered
	EGG_CASE2_DEFINED,		// genmyc.c, planet 1 shattered
	PKUNK_DEFINED,			// genpku.c, planet 1 [habitable] (water)
	UTWIG_DEFINED,			// genutw.c, planet 1 [habitable] (water????)
	SUPOX_DEFINED,			// gensup.c, planet 1 [habitable] (water)
	YEHAT_DEFINED,			// genyeh.c, planet 1 [habitable] (water)
	VUX_DEFINED,			// genvux.c, planet 1 [habitable] (redux)
	ORZ_DEFINED,			// genorz.c, planet 1 [habitable] (water)
	THRADD_DEFINED,			// genthrad.c, planet 1 [habitable] (water)
	RAINBOW0_DEFINED,
	RAINBOW1_DEFINED,
	RAINBOW2_DEFINED,
	RAINBOW3_DEFINED,
	RAINBOW4_DEFINED,
	RAINBOW5_DEFINED,
	RAINBOW6_DEFINED,
	RAINBOW7_DEFINED,
	RAINBOW8_DEFINED,
	RAINBOW9_DEFINED,
	ILWRATH_DEFINED,		// genilw.c, planet 1 [habitable] (primordial)
	ANDROSYNTH_DEFINED,		// genand.c, planet 2/rand [habitable] (telluric)
	MYCON_TRAP_DEFINED,		// gentrap.c, planet 1 [habitable] (telluric)
	URQUAN0_DEFINED,		// gensam.c, planet random
	URQUAN1_DEFINED,		// gensam.c, planet random
	URQUAN2_DEFINED,		// gensam.c, planet random
	KOHRAH0_DEFINED,		// gensam.c, planet random
	KOHRAH1_DEFINED,		// gensam.c, planet random
	KOHRAH2_DEFINED,		// gensam.c, planet random
	DESTROYED_STARBASE_DEFINED,// gensam.c, planet 1 [habitable] (primordial)
	MOTHER_ARK_DEFINED,		// genchmmr.c, planet 4/rand [crystal] (emerald)
	ZOQ_COLONY0_DEFINED,	// genzoq.c, planet 0 [habitable] (redux)
	ZOQ_COLONY1_DEFINED,	// genzoq.c, planet 1 [habitable] (redux)
	ZOQ_COLONY2_DEFINED,	// genzoq.c, planet 1 moon rand [habitable] (redux)
	ZOQ_COLONY3_DEFINED,	// genzoq.c, planet 0 [habitable] (redux)
	ALGOLITES_DEFINED,		// genspa.c, planet 4 [habitable, airless]
	SPATHI_MONUMENT_DEFINED,// genspa.c, planet random [crystal]
	EXCAVATION_SITE_DEFINED,// genand.c, planet 1/rand [any rocky]
	NUM_PLOTS,				// Must be right after last valid entry.  Any new
	WAR_ERA = NUM_PLOTS,	// entries need plot min/max added in gendef.c
	SUCCESS = NUM_PLOTS,
	HOME = NUM_PLOTS + 1
};

#define UMGAH_DEFINED TALKING_PET_DEFINED

#if defined(__cplusplus)
}
#endif

#endif  /* GENDEF_H */

