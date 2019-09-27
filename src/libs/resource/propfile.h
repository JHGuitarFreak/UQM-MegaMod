/*  propfile.h, Copyright (c) 2008 Michael C. Martin */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope thta it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Se the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PROPFILE_H_
#define PROPFILE_H_

#include "libs/uio.h"

typedef void (*PROPERTY_HANDLER) (const char *, const char *);

void PropFile_from_string (char *d, PROPERTY_HANDLER handler, const char *prefix);
void PropFile_from_file (uio_Stream *f, PROPERTY_HANDLER handler, const char *prefix);
void PropFile_from_filename (uio_DirHandle *path, const char *fname, PROPERTY_HANDLER handler, const char *prefix);

#endif
