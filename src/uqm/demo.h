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

#ifndef UQM_DEMO_H_
#define UQM_DEMO_H_

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef DEMO_MODE
#define DEMO_MODE 0
#endif /* DEMO_MODE */
#ifndef CREATE_JOURNAL
#define CREATE_JOURNAL 0
#endif /* CREATE_JOURNAL */

#if !(DEMO_MODE || CREATE_JOURNAL)

#define OpenJournal SeedRandomNumbers
#define CloseJournal() TRUE
#define JournalInput(is)

#else

extern void OpenJournal (void);
extern BOOLEAN CloseJournal (void);
#if !CREATE_JOURNAL
#define JournalInput(is)
#else /* CREATE_JOURNAL */
extern void JournalInput (INPUT_STATE InputState);
#endif /* CREATE_JOURNAL */

#endif

#if defined(__cplusplus)
}
#endif

#endif /* UQM_DEMO_H_ */

