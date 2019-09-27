/*
 * Copyright (C) 2003  Serge van den Boom
 *
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

#ifndef LIBS_UIO_MOUNTTREE_H_
#define LIBS_UIO_MOUNTTREE_H_

#include <stdio.h>
#include "mount.h"

void uio_printMounts(FILE *outStream, const uio_Repository *repository);

/* *** Internal definitions follow *** */
#ifdef uio_INTERNAL

#include <sys/types.h>

typedef struct uio_MountTreeItem uio_MountTreeItem;
typedef struct uio_MountTree uio_MountTree;
typedef struct uio_MountInfo uio_MountInfo;

#include "physical.h"
#include "types.h"
#include "uioport.h"
#include "paths.h"


/*
 * A MountTree describes the relation between dirs (PRoot structures)
 * mounted in a directory structure.
 * A MountTree structure represents a node (a dir) in this directory
 * structure.
 * It describes what MountInfo structures apply in that dir and below (if
 * not overrided in subnodes) (the 'pLocs' field).
 * These path components are also linked together 'up'-wards (towards the
 * root of the tree) by the 'up' field.
 */

struct uio_MountTreeItem {
	struct uio_MountInfo *mountInfo;
	int depth;
			// 'mountInfo->pDirHandle' and 'depth' together point to a
			// location in a physical tree. An uio_pDirHandle alone can't be
			// used as the directory might not exist.
			// So pDirHandle points to the top dir that was mounted, and
			// 'depth' indicates how many directory names of the path to
			// this point in the Mount Tree need to be followed.
			// Example:
			// This MountTreeItem is somewhere in a tree /foo/bar/bla
			// and depth = 1. Then this MountTreeItem points to
			// /bla in the specified root.
	struct uio_MountTreeItem *next;
			// The next MountTreeItem in a MountTree
};

struct uio_MountTree {
	struct uio_MountTree *subTrees;
			// Trees for subdirs in this MountTree
	struct uio_MountTreeItem *pLocs;
			// The physical locations that have effect in this MountTree.
	struct uio_MountTree *upTree;
			// the MountTree that pointed to this MountTree
	struct uio_PathComp *comps;
			// the names of the path components that lead to the tree.
			// Not necessary every PathComp is connected to a MountTree.
			// If you have /foo and /foo/bar/zut mounted, then
			// there are MountTrees for /,  /foo and /foo/bar/zut,
			// but there are PathComps for 'foo', 'bar' and 'zut'.
	struct uio_PathComp *lastComp;
			// The last PathComp of comps that pointed to this MountTree.
			// This can be used to trace the path back to the top.
	struct uio_MountTree *next;
			// If this tree is a subTree of a tree, 'next' points to the
			// next subTree of that tree.
};

/*
 * A MountInfo structure describes how a physical structure was mounted.
 * A physical structure can be used by several MountInfo structures.
 */
struct uio_MountInfo {
	int flags;
			/* Mount flags */
#	define uio_MOUNTINFO_RDONLY uio_MOUNT_RDONLY
	uio_FileSystemID fsID;
	char *dirName;
			/* The path inside the mounted fs leading to pDirHandle */
	uio_PDirHandle *pDirHandle;
			/* The pDirHandle belonging to this mount */
	uio_MountTree *mountTree;
			/* The MountTree node for the mountpoint */
	uio_AutoMount **autoMount;
	uio_MountHandle *mountHandle;
};


/*
 *	Say we've got mounted (in order):
 *	Bla -> /
 *	Bar -> /foo/bar
 *	Foo -> /foo
 *	Zut -> /zut/123
 *	Fop -> /zut/123
 *
 *  This will build a tree that looks like this:
 *  (the strings between brackets are the mounted filesystems that have effect
 *   in a dir, in order)
 *
 *  / (Bar)
 *  	foo (Foo, Bla)
 *  		bar (Foo, Bar, Bla)
 *		zut/123 (Fop, Zut, Bla)
 * 
 *	The MountTree will look like:
 *	/ = {
 *		sub = {
 *			/foo,
 *			/zut/123
 *		},
 *	    pLocs = {
 *	    	BlaDir:/ (0)
 *	    }
 *	}
 *	/foo = {
 *		sub = {
 *			/foo/bar
 *		},
 *		pLocs = {
 *			BlaDir:/foo (1),
 *			FooDir:/ (0)
 *		}
 *	}
 *	/foo/bar = {
 *		sub = { },
 *	    pLocs = {
 *	    	BlaDir:/foo/bar (2),
 *	    	BarDir:/ (0),
 *	    	FooDir:/bar (1)
 *	    }
 *	}
 *	/zut/123 = {
 *		sub = { },
 *		pLocs = {
 *          FooDir:/ (0)
 *          ZutDir:/ (0)
 *			BlaDir:/zut/123 (2)
 *		}
 *	}
 *
 *	'BlaDir:/zut/123 (2)' means the pDirHandle is 'Bla', and the dir into
 *  that directory is '/zut/123', but as this is always a postfix of the
 *  path where we are, the number of dirs (the '(2)') is enough to store
 *  (apart from the pDirHandle).
 */

uio_MountTree *uio_makeRootMountTree(void);
void uio_MountTree_delete(uio_MountTree *tree);
uio_MountTree *uio_mountTreeAddMountInfo(uio_Repository *repository,
		uio_MountTree *mountTree, uio_MountInfo *mountInfo, const char *path,
		uio_MountLocation location, const uio_MountInfo *relative);
void uio_mountTreeRemoveMountInfo(uio_Repository *repository,
		uio_MountTree *mountTree, uio_MountInfo *mountInfo);
void uio_findMountTree(uio_MountTree *top, const char *path,
		uio_MountTree **resTree, const char **pPath);
char *uio_mountTreeItemRestPath(const uio_MountTreeItem *item,
		uio_PathComp *endComp, const char *path);
int uio_mountTreeCountPLocs(const uio_MountTree *tree);
uio_MountInfo *uio_MountInfo_new(uio_FileSystemID fsID,
		uio_MountTree *mountTree, uio_PDirHandle *pDirHandle,
		char *dirName, uio_AutoMount **autoMount,
		uio_MountHandle *mountHandle, int flags);
void uio_MountInfo_delete(uio_MountInfo *mountInfo);
void uio_printMountTree(FILE *outStream, const uio_MountTree *tree,
		int indent);
void uio_printMountTreeItem(FILE *outStream, const uio_MountTreeItem *item);
void uio_printMountTreeItems(FILE *outStream, const uio_MountTreeItem *item);
void uio_printPathToMountTree(FILE *outStream, const uio_MountTree *tree);
void uio_printMountInfo(FILE *outStream, const uio_MountInfo *mountInfo);

static inline uio_bool
uio_mountInfoIsReadOnly(uio_MountInfo *mountInfo) {
	return (mountInfo->flags & uio_MOUNTINFO_RDONLY) != 0;
}

#endif  /* uio_INTERNAL */

#endif  /* LIBS_UIO_MOUNTTREE_H_ */

