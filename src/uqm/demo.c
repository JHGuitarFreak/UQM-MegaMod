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

#include "demo.h"
#include "libs/declib.h"
#include "setup.h"

#if DEMO_MODE || CREATE_JOURNAL
static DECODE_REF journal_fh;
static char journal_buf[1024
#if CREATE_JOURNAL
								* 8
#else /* DEMO_MODE */
								* 2
#endif
								];
INPUT_REF DemoInput;
#endif

#if DEMO_MODE

static INPUT_REF OldArrowInput;

INPUT_STATE
demo_input (INPUT_REF InputRef, INPUT_STATE InputState)
{
	if (InputState || AnyButtonPress () || cread (
			&InputState, sizeof (InputState), 1, journal_fh
			) == 0)
	{
		cclose (journal_fh);
		journal_fh = 0;

		StopMusic ();
		StopSound ();

		FreeKernel ();
		exit (1);
	}

	return (InputState);
}

#endif /* DEMO_MODE */

#if CREATE_JOURNAL

void
JournalInput (INPUT_STATE InputState)
{
	if (ArrowInput != DemoInput && journal_fh)
		cwrite (&InputState, sizeof (InputState), 1, journal_fh);
}

#endif /* CREATE_JOURNAL */

#if DEMO_MODE || CREATE_JOURNAL

void
OpenJournal (void)
{
	DWORD start_seed;

#if CREATE_JOURNAL
	if (create_journal)
	{
		if (journal_fh = copen (journal_buf, MEMORY_STREAM, STREAM_WRITE))
		{
			start_seed = SeedRandomNumbers ();
			cwrite (&start_seed, sizeof (start_seed), 1, journal_fh);
		}
	}
	else
#endif /* CREATE_JOURNAL */
	{
		uio_Stream *fp;

		if (fp = res_OpenResFile ("starcon.jnl", "rb"))
		{
			ReadResFile (journal_buf, 1, sizeof (journal_buf), fp);
			res_CloseResFile (fp);

			if (journal_fh = copen (journal_buf, MEMORY_STREAM, STREAM_READ))
			{
				OldArrowInput = ArrowInput;
				ArrowInput = DemoInput;
				PlayerInput[0] = PlayerInput[1] = DemoInput;

				FlushInput ();

				cread (&start_seed, sizeof (start_seed), 1, journal_fh);
				TFB_SeedRandom (start_seed);
			}
		}
	}
}

BOOLEAN
CloseJournal (void)
{
	if (journal_fh)
	{
		uio_Stream *fp;

		cclose (journal_fh);
		journal_fh = 0;

		if (ArrowInput == DemoInput)
		{
			ArrowInput = OldArrowInput;
			return (FALSE);
		}
#if CREATE_JOURNAL
		else if (fp = res_OpenResFile ("starcon.jnl", "wb"))
		{
			WriteResFile (journal_buf, 1, sizeof (journal_buf), fp);
			res_CloseResFile (fp);
		}
#endif /* CREATE_JOURNAL */
	}

	return (TRUE);
}

#endif

