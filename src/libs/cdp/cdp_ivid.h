/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * CDP Video Interface 
 */

#ifndef LIBS_CDP_CDP_IVID_H_
#define LIBS_CDP_CDP_IVID_H_

#include "types.h"
#include "libs/video/video.h"
#include "libs/video/videodec.h"

// CDP Video Interface entry points
typedef struct
{
	TFB_RegVideoDecoder* (* RegisterDecoder) (const char* fileext,
			TFB_VideoDecoderFuncs*);
	void (* UnregisterDecoder) (TFB_RegVideoDecoder*);
	const TFB_VideoDecoderFuncs* (* LookupDecoder) (const char* fileext);

} cdp_Itf_VideoVtbl_v1;

// the following are for the sake of module writers
typedef cdp_Itf_VideoVtbl_v1 cdp_Itf_VideoVtbl;
typedef cdp_Itf_VideoVtbl    cdp_Itf_Video;

#endif  /* LIBS_CDP_CDP_IVID_H_ */
