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

#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#	include <stdio.h>
#endif
#include <assert.h>

#include "iointrn.h"
#include "uioport.h"
#include "mounttree.h"
#include "paths.h"
#include "types.h"
#include "mem.h"
#include "uioutils.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

static uio_MountTree *uio_mountTreeAddMountInfoRecTree(
		uio_Repository *repository, uio_MountTree *tree,
		uio_MountInfo *mountInfo, const char *start, const char *end,
		uio_PathComp *upComp, uio_MountLocation location,
		const uio_MountInfo *relative);
static inline uio_MountTree *uio_mountTreeAddMountInfoRecTreeSub(
		uio_Repository *repository, uio_MountTree **tree,
		uio_MountInfo *mountInfo, const char *start,
		const char *end, uio_MountLocation location,
		const uio_MountInfo *relative);
static void uio_mountTreeAddMountInfoLocAll(uio_Repository *repository,
		uio_MountTree *tree, uio_MountInfo *mountInfo, int depth,
		uio_MountLocation location, const uio_MountInfo *relative);
static uio_MountTreeItem *uio_copyMountTreeItems(uio_MountTreeItem *item,
		int extraDepth);
static void uio_addMountTreeItem(uio_Repository *repository,
		uio_MountTreeItem **pLocs, uio_MountTreeItem *item,
		uio_MountLocation location, const uio_MountInfo *relative);
static uio_MountTree *uio_mountTreeAddNewSubTree(uio_Repository *repository,
		uio_MountTree *tree, const char *path, uio_MountInfo *mountInfo,
		uio_PathComp *upComp, uio_MountLocation location,
		const uio_MountInfo *relative);
static void uio_mountTreeAddSub(uio_MountTree *tree, uio_MountTree *sub);
static uio_MountTree * uio_splitMountTree(uio_MountTree **tree, uio_PathComp
		*lastComp, int depth);
static void uio_mountTreeRemoveMountInfoRec(uio_MountTree *mountTree,
		uio_MountInfo *mountInfo);
static void uio_printMount(FILE *outStream, const uio_MountInfo *mountInfo);

static inline uio_MountTree * uio_MountTree_new(uio_MountTree *subTrees,
		uio_MountTreeItem *pLocs, uio_MountTree *upTree, uio_PathComp
		*comps, uio_PathComp *lastComp, uio_MountTree *next);
static inline uio_MountTreeItem *uio_MountTree_newItem(
		uio_MountInfo *mountInfo, int depth, uio_MountTreeItem *next);

static inline void uio_MountTreeItem_delete(uio_MountTreeItem *item);

static inline uio_MountTree *uio_MountTree_alloc(void);
static inline uio_MountTreeItem *uio_MountTreeItem_alloc(void);
static inline uio_MountInfo *uio_MountInfo_alloc(void);

static inline void uio_MountTree_free(uio_MountTree *mountTree);
static inline void uio_MountTreeItem_free(uio_MountTreeItem *mountTreeItem);
static inline void uio_MountInfo_free(uio_MountInfo *mountInfo);


// make the root mount Tree
uio_MountTree *
uio_makeRootMountTree(void) {
	return uio_MountTree_new(NULL, NULL, NULL, NULL, NULL, NULL);
}

// Add a MountInfo structure to a MountTree in the place pointed
// to by 'path'.
// returns the MountTree for the location at the end of the path
uio_MountTree *
uio_mountTreeAddMountInfo(uio_Repository *repository, uio_MountTree *mountTree,
		uio_MountInfo *mountInfo, const char *path, uio_MountLocation location,
		const uio_MountInfo *relative) {
	const char *start, *end;

	getFirstPath0Component(path, &start, &end);
	return uio_mountTreeAddMountInfoRecTree(repository, mountTree, mountInfo,
			start, end, NULL, location, relative);
}

// recursive helper for uio_mountTreeAddMountInfo
// returns the MountTree for the location at the end of the path
static uio_MountTree *
uio_mountTreeAddMountInfoRecTree(uio_Repository *repository, uio_MountTree *tree,
		uio_MountInfo *mountInfo, const char *start, const char *end,
		uio_PathComp *upComp,
		uio_MountLocation location, const uio_MountInfo *relative) {
	uio_MountTree **sub;
	
	if (*start == '\0') {
		// End of the path. Put the MountInfo here and on all subtrees below
		// this level.
		uio_mountTreeAddMountInfoLocAll(repository, tree, mountInfo, 0,
				location, relative);
		return tree;
	}

	// Check if sub trees match the path.
	for (sub = &tree->subTrees; *sub != NULL; sub = &(*sub)->next) {
		uio_MountTree *resTree;

		resTree = uio_mountTreeAddMountInfoRecTreeSub(repository, sub,
				mountInfo, start, end, location, relative);
		if (resTree != NULL) {
			// handled
			return resTree;
		}
	}
	// No subtree found matching (part of) 'path'.

	// Need to add a new tree sub
	return uio_mountTreeAddNewSubTree(repository, tree, start, mountInfo,
			upComp, location, relative);
}

// recursive helper for uio_mountTreeAddMountInfo
// Pre: *start != '\0'
// returns the MountTree for the location at the end of the path, if
// that falls within this tree. If not, returns NULL.
static inline uio_MountTree *
uio_mountTreeAddMountInfoRecTreeSub(uio_Repository *repository,
		uio_MountTree **tree, uio_MountInfo *mountInfo,
		const char *start, const char *end, uio_MountLocation location,
		const uio_MountInfo *relative) {
	uio_PathComp *comp, *lastComp;
	int depth;
	
	comp = (*tree)->comps;
	if (strncmp(comp->name, start, end - start) != 0 ||
			comp->name[end - start] != '\0') {
		// first component does not match; this is not the correct subTree
		return NULL;
	}
	
	depth = 1;
	// try to match all components of the directory path to this subTree.
	while (1) {
		getNextPath0Component(&start, &end);
		lastComp = comp;
		comp = comp->next;
	
		if (comp == NULL)
			break;
		
		if (*start == '\0') {
			// end of the path reached
			// We need to split up the components and insert a new
			// MountTree here.
			uio_MountTree *newTree;
			newTree = uio_splitMountTree(tree, lastComp, depth);
			
			// Add mountInfo to each of the MountTrees below newTree.
			uio_mountTreeAddMountInfoLocAll(repository, newTree, mountInfo, 0,
					location, relative);

			return newTree;
		}
		if (strncmp(comp->name, start, end - start) != 0 ||
				comp->name[end - start] != '\0') {
			// Some, but not all components matched; we need to split
			// up the components and add a new subTree here for the
			// (non-matching) rest of the path.
			uio_MountTree *newTree;
			
			newTree = uio_splitMountTree(tree, lastComp, depth);

			// A new Tree is added at the split-point.
			return uio_mountTreeAddNewSubTree(repository, newTree, start,
					mountInfo, lastComp, location, relative);
		}
		getNextPath0Component(&start, &end);
		depth++;
	}

	// All components matched. We can recurse to the next subdir.
	return uio_mountTreeAddMountInfoRecTree(repository, *tree, mountInfo,
			start, end, lastComp, location, relative);
}

// Add a MountInfo struct 'mountInfo' to the pLocs fields of all subTrees
// starting with 'tree'.
// 'depth' is the distance to the MountTree where the MountInfo is located.
static void
uio_mountTreeAddMountInfoLocAll(uio_Repository *repository, uio_MountTree *tree,
		uio_MountInfo *mountInfo, int depth, uio_MountLocation location,
		const uio_MountInfo *relative) {
	uio_MountTreeItem *newPLoc;
	uio_MountTree *subTree;
	int compCount;

	// Add a new PLoc to this mountTree
	newPLoc = uio_MountTree_newItem(mountInfo, depth, NULL);
	uio_addMountTreeItem(repository, &tree->pLocs, newPLoc, location, relative);
	
	// Recurse for subtrees
	for (subTree = tree->subTrees; subTree != NULL;
			subTree = subTree->next) {
		compCount = uio_countPathComps(subTree->comps);
		uio_mountTreeAddMountInfoLocAll(
				repository, subTree, mountInfo, depth + compCount,
				location, relative);
	}
}

// pre: repository->mounts is already updated
// pre: if location is uio_MOUNT_BELOW or uio_MOUNT_ABOVE, 'relative'
//      exists in repository->mounts
static void
uio_addMountTreeItem(uio_Repository *repository, uio_MountTreeItem **pLocs,
		uio_MountTreeItem *item,
		uio_MountLocation location, const uio_MountInfo *relative) {
	switch (location) {
		case uio_MOUNT_TOP:
			item->next = *pLocs;
			*pLocs = item;
			break;
		case uio_MOUNT_BOTTOM:
			while (*pLocs != NULL)
				pLocs = &(*pLocs)->next;
			item->next = NULL;
			*pLocs = item;
			break;
		case uio_MOUNT_ABOVE: {
			uio_MountInfo **mountInfo;
			mountInfo = repository->mounts;
			while (*mountInfo != relative) {
				assert(*mountInfo != NULL);
				if ((*pLocs)->mountInfo == *mountInfo)
					pLocs = &(*pLocs)->next;
				mountInfo++;
			}
			item->next = *pLocs;
			*pLocs = item;
			break;
		}
		case uio_MOUNT_BELOW: {
			uio_MountInfo **mountInfo;
			mountInfo = repository->mounts;
			while (*mountInfo != relative) {
				assert(*mountInfo != NULL);
				if ((*pLocs)->mountInfo == *mountInfo)
					pLocs = &(*pLocs)->next;
				mountInfo++;
			}
			item->next = (*pLocs)->next;
			(*pLocs)->next = item;
			break;
		}
		default:
			assert(false);
	}
}

// Copy a chain of MountTreeItems, but increase the depth by 'extraDepth'.
static uio_MountTreeItem *
uio_copyMountTreeItems(uio_MountTreeItem *item, int extraDepth) {
	uio_MountTreeItem *result, **resPtr;
	uio_MountTreeItem *newItem;

	resPtr = &result;
	while (item != NULL) {
		newItem = uio_MountTree_newItem(
				item->mountInfo, item->depth + extraDepth, NULL);
		*resPtr = newItem;
		resPtr = &newItem->next;
		item = item->next;
	}
	*resPtr = NULL;
	return result;
}

// add a new sub tree under a tree 'tree'.
// 'path' is the part leading up to the new tree and
// 'mountInfo' is the MountInfo structure to at there.
// 'upComp' points to the last path component that lead to 'tree'.
static uio_MountTree *
uio_mountTreeAddNewSubTree(uio_Repository *repository, uio_MountTree *tree,
		const char *path, uio_MountInfo *mountInfo, uio_PathComp *upComp,
		uio_MountLocation location, const uio_MountInfo *relative) {
	uio_MountTreeItem *item, *items;
	uio_MountTree *newTree;
	uio_PathComp *compList, *lastComp;
	int compCount;
	
	compList = uio_makePathComps(path, upComp);
	compCount = uio_countPathComps(compList);
	lastComp = uio_lastPathComp(compList);
	item = uio_MountTree_newItem(mountInfo, 0, NULL);
	item->next = NULL;
	items = uio_copyMountTreeItems(tree->pLocs, compCount);
	uio_addMountTreeItem(repository, &items, item, location,
			relative);
	newTree = uio_MountTree_new(
			NULL /* subTrees */,
			items /* pLocs */,
			tree /* upTree */,
			compList /* comps */,
			lastComp /* lastComp */,
			NULL /* next */);
	uio_mountTreeAddSub(tree, newTree);
	return newTree;
}

// add a sub structure to the end of the 'subTrees' list of a tree.
static void
uio_mountTreeAddSub(uio_MountTree *tree, uio_MountTree *sub) {
	uio_MountTree **subPtr;
	
	for (subPtr = &tree->subTrees; *subPtr != NULL;
			subPtr = &(*subPtr)->next) {
		// Nothing to do here.
	}
	*subPtr = sub;
}

// Add a new MountTree structure in between two MountTrees.
// Tree points to the pointer for the tree in front of which the new
// tree needs to be placed (at depth 'depth').
// 'lastComp' is the last pathComp of the part before the splitting point
// It returns the new MountTree.
static uio_MountTree *
uio_splitMountTree(uio_MountTree **tree, uio_PathComp *lastComp, int depth) {
	uio_MountTree *newTree;
	uio_MountTreeItem *items;

	items = uio_copyMountTreeItems((*tree)->upTree->pLocs, depth);
	newTree = uio_MountTree_new(
			*tree /* subTrees */,
			items /* pLocs */,
			(*tree)->upTree /* upTree */,
			(*tree)->comps /* comps */,
			lastComp /* lastComp */,
			NULL /* next */);
	(*tree)->upTree = newTree;
	(*tree)->comps = lastComp->next;
	lastComp->next = NULL;
	*tree = newTree;
	return newTree;
}

void
uio_mountTreeRemoveMountInfo(uio_Repository *repository,
		uio_MountTree *mountTree, uio_MountInfo *mountInfo) {
	uio_MountTree **subTreePtr;
	uio_MountTree *upTree;

	// If the tree has no sub-trees and it has the same items as the
	// upTree, with 'mountInfo' added, then the tree is a dead end
	// and can be removed entirely.
	// First we handle the other case.
	// Note that if the upTree has exactly one item less than the tree
	// itself, these items must be the same, plus mountInfo for the
	// tree itself, as each tree has at least the items of its upTree.
	if (mountTree->upTree == NULL || mountTree->subTrees != NULL ||
			uio_mountTreeCountPLocs(mountTree) !=
			uio_mountTreeCountPLocs(mountTree->upTree) + 1) {
		// We can't remove the tree itself.
		// We need to remove the mountInfo from the tree, and all subTrees.
		// Then we're done.
		uio_mountTreeRemoveMountInfoRec(mountTree, mountInfo);
		return;
	}

	// mountTree itself can be removed.
	// First remove the tree from the list of subtrees of the upTree.
	subTreePtr = &mountTree->upTree->subTrees;
	while (1) {
		assert(*subTreePtr != NULL);
		if (*subTreePtr == mountTree)
			break;
		subTreePtr = &(*subTreePtr)->next;
	}
	*subTreePtr = mountTree->next;

	// Save the upTree for later.
	upTree = mountTree->upTree;
	
	// Remove the tree itself.
	uio_MountTree_delete(mountTree);

	// The upTree itself could have become unnecessary now.
	// This is the case when upTree now only has one subTree, and upTree
	// and the subTree have the same items.
	// Again, same item count implies same items.
	if (upTree->subTrees == NULL || upTree->subTrees->next != NULL ||
			uio_mountTreeCountPLocs(upTree) !=
			uio_mountTreeCountPLocs(upTree->subTrees)) {
		// upTree is still necessary. We're done.
		return;
	}

	// Merge upTree and upTree->subTrees.
	// It would be easiest to keep upTree, and throw upTree->subTrees away,
	// but that's not possible as external links point to upTree->subTrees.
	// First merge the path components:
	assert(upTree->subTrees->lastComp != NULL);
	upTree->subTrees->lastComp->next = upTree->subTrees->comps;
	upTree->subTrees->lastComp = upTree->lastComp;
	upTree->subTrees->comps = upTree->comps;
	// Now let the pointer that pointed to upTree, point to upTree->subTrees.
	// Change upTree->next accordingly.
	if (upTree->upTree == NULL) {
		assert(repository->mountTree == upTree);
		repository->mountTree = upTree->subTrees;
		// upTree->subTrees->next is already NULL
	} else {
		uio_MountTree *next;
		subTreePtr = &upTree->upTree->subTrees;
		while (1) {
			assert(*subTreePtr != NULL);
			if (*subTreePtr == upTree)
				break;
			subTreePtr = &(*subTreePtr)->next;
		}
		next = (*subTreePtr)->next;
		*subTreePtr = upTree->subTrees;
		upTree->subTrees->next = next;
	}

	// Now delete the tree itself
	upTree->subTrees = NULL;
	upTree->comps = NULL;
	uio_MountTree_delete(upTree);	
}

// pre: mountInfo exists in mountTree->pLocs (and hence in pLocs for
// every sub-tree)
static void
uio_mountTreeRemoveMountInfoRec(uio_MountTree *mountTree,
		uio_MountInfo *mountInfo) {
	uio_MountTree *subTree;
	uio_MountTreeItem **itemPtr, *item;

	// recurse for all subTrees
	for (subTree = mountTree->subTrees; subTree != NULL;
			subTree = subTree->next)
		uio_mountTreeRemoveMountInfoRec(subTree, mountInfo);

	// Find the mount info in this tree.
	itemPtr = &mountTree->pLocs;
	while (1) {
		assert(*itemPtr != NULL);
				// We know an item with the specified mountInfo
				// must be here somewhere.
		if ((*itemPtr)->mountInfo == mountInfo) {
			// Found it.
			break;
		}
		itemPtr = &(*itemPtr)->next;
	}

	item = *itemPtr;
	*itemPtr = item->next;
	uio_MountTreeItem_delete(item);
}

// Count the number of pLocs in a tree that leads to.
int
uio_mountTreeCountPLocs(const uio_MountTree *tree) {
	int count;
	uio_MountTreeItem *item;
	
	count = 0;
	for (item = tree->pLocs; item != NULL; item = item->next)
		count++;
	return count;
}

// resTree may point to top
// pPath may point to path
void
uio_findMountTree(uio_MountTree *top, const char *path,
		uio_MountTree **resTree, const char **pPath) {
	const char *start, *end, *pathFromTree;
	uio_MountTree *tree, *sub;
	uio_PathComp *comp;
	
	getFirstPath0Component(path, &start, &end);
	tree = top;
	while(1) {
		if (*start == '\0') {
			*resTree = tree;
			*pPath = start;
			return;
		}
		
		pathFromTree = start;
		sub = tree->subTrees;
		while(1) {
			if (sub == NULL) {
				// No matching sub Dirs found. So we report back the current
				// dir.
				*resTree = tree;
				*pPath = pathFromTree;
				return;
			}
			comp = sub->comps;
			if (strncmp(comp->name, start, end - start) == 0 &&
					comp->name[end - start] == '\0')
				break;
			sub = sub->next;
		}
		// Found a Sub dir which matches at least partially.

		while (1) {
			getNextPath0Component(&start, &end);
			comp = comp->next;
			if (comp == NULL)
				break;
			if (*start == '\0' ||
					strncmp(comp->name, start, end - start) != 0 ||
					comp->name[end - start] != '\0') {
				// either the path ends here, or the path in the tree does.
				// either way, the last Tree is the one we want.
				*resTree = tree;
				*pPath = pathFromTree;
				return;
			}
		}
		// all components matched until the next MountTree
		tree = sub;
	}
}

// finds the path to the MountInfo associated with a mountTreeItem
// given a path to the 'item' itself.
// 'item' is the mountTreeItem
// 'endComp' is the last PathComp leading to 'item'
// 'start' is the start of the path to the item
char *
uio_mountTreeItemRestPath(const uio_MountTreeItem *item,
		uio_PathComp *endComp, const char *path) {
	int i;
	const char *pathPtr;
	
	i = item->depth;
	while (i--)
		endComp = endComp->up;

	pathPtr = path;
	if (endComp != NULL) {
		while (1) {
			pathPtr += endComp->nameLen;
			endComp = endComp->up;
			if (endComp == NULL)
				break;
			pathPtr++;
					// for a '/'
		}
	}
	if (*path == '/') {
		// / at the beginning of the path
		pathPtr++;
	}
	if (*pathPtr == '/') {
		// / at the end of the path
		pathPtr++;
	}
//	return (char *) pathPtr;
			// gives warning
//	return *((char **)((void *) &pathPtr));
			// not portable
	return (char *) unconst((const void *) pathPtr);
}

void
uio_printMountTree(FILE *outStream, const uio_MountTree *tree, int indent) {
	uio_MountTree *sub;
	uio_PathComp *comp;
	
	fprintf(outStream, "(");
	uio_printMountTreeItems(outStream, tree->pLocs);
	fprintf(outStream, ")\n");
	for (sub = tree->subTrees; sub != NULL; sub = sub->next) {
		int newIndent;

		newIndent = indent;
		fprintf(outStream, "%*s", indent, "");
		for (comp = sub->comps; comp != NULL; comp = comp->next) {
			fprintf(outStream, "/%s", comp->name);
			newIndent += 1 + comp->nameLen;
		}
		fprintf(outStream, " ");
		newIndent += 1;
		uio_printMountTree(outStream, sub, newIndent);
	}
}

void
uio_printMountTreeItem(FILE *outStream, const uio_MountTreeItem *item) {
	uio_printMountInfo(outStream, item->mountInfo);
	fprintf(outStream, ":%d", item->depth);
}

void
uio_printMountTreeItems(FILE *outStream, const uio_MountTreeItem *item) {
	if (!item)
		return;
	while(1) {
		uio_printMountTreeItem(outStream, item);
		item = item->next;
		if (item == NULL)
			break;
		fprintf(outStream, ", ");
	}
}

void
uio_printPathToMountTree(FILE *outStream, const uio_MountTree *tree) {
	if (tree->upTree == NULL) {
		fprintf(outStream, "/");
	} else
		uio_printPathToComp(outStream, tree->lastComp);
}

void
uio_printMountInfo(FILE *outStream, const uio_MountInfo *mountInfo) {
	uio_FileSystemInfo *fsInfo;
	
	fsInfo = uio_getFileSystemInfo(mountInfo->fsID);
	fprintf(outStream, "%s:/%s", fsInfo->name, mountInfo->dirName);
}

static void
uio_printMount(FILE *outStream, const uio_MountInfo *mountInfo) {
	uio_FileSystemInfo *fsInfo;
	
	fsInfo = uio_getFileSystemInfo(mountInfo->fsID);
	fprintf(outStream, "???:%s on ", mountInfo->dirName);
	uio_printPathToMountTree(outStream, mountInfo->mountTree);
	fprintf(outStream, " type %s (", fsInfo->name);
	if (mountInfo->flags & uio_MOUNT_RDONLY) {
		fprintf(outStream, "ro");
	} else
		fprintf(outStream, "rw");
	fprintf(outStream, ")\n");
}

void
uio_printMounts(FILE *outStream, const uio_Repository *repository) {
	int i;

	for (i = 0; i < repository->numMounts; i++) {
		uio_printMount(outStream, repository->mounts[i]);
	}
}


// *** uio_MountTree*** //

static inline uio_MountTree *
uio_MountTree_new(uio_MountTree *subTrees, uio_MountTreeItem *pLocs,
		uio_MountTree *upTree, uio_PathComp *comps, uio_PathComp *lastComp,
		uio_MountTree *next) {
	uio_MountTree *result;
	
	result = uio_MountTree_alloc();
	result->subTrees = subTrees;
	result->pLocs = pLocs;
	result->upTree = upTree;
	result->comps = comps;
	result->lastComp = lastComp;
	result->next = next;
	return result;
}

void
uio_MountTree_delete(uio_MountTree *tree) {
	uio_MountTree *subTree, *nextTree;
	uio_MountTreeItem *item, *nextItem;

	subTree = tree->subTrees;
	while (subTree != NULL) {
		nextTree = subTree->next;
		uio_MountTree_delete(subTree);
		subTree = nextTree;
	}

	item = tree->pLocs;
	while (item != NULL) {
		nextItem = item->next;
		uio_MountTreeItem_delete(item);
		item = nextItem;
	}

	if (tree->comps != NULL)
		uio_PathComp_delete(tree->comps);

	uio_MountTree_free(tree);
}

static inline uio_MountTree *
uio_MountTree_alloc(void) {
	uio_MountTree *result = uio_malloc(sizeof (uio_MountTree));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_MountTree, (void *) result);
#endif
	return result;
}

static inline void
uio_MountTree_free(uio_MountTree *mountTree) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_MountTree, (void *) mountTree);
#endif
	uio_free(mountTree);
}


// *** uio_MountTreeItem *** //

static inline uio_MountTreeItem *
uio_MountTree_newItem(uio_MountInfo *mountInfo, int depth,
		uio_MountTreeItem *next) {
	uio_MountTreeItem *result;
	
	result = uio_MountTreeItem_alloc();
	result->mountInfo = mountInfo;
	result->depth = depth;
	result->next = next;
	return result;
}

static inline void
uio_MountTreeItem_delete(uio_MountTreeItem *item) {
	uio_MountTreeItem_free(item);
}

static inline uio_MountTreeItem *
uio_MountTreeItem_alloc(void) {
	uio_MountTreeItem *result = uio_malloc(sizeof (uio_MountTreeItem));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_MountTreeItem, (void *) result);
#endif
	return result;
}

static inline void
uio_MountTreeItem_free(uio_MountTreeItem *mountTreeItem) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_MountTreeItem, (void *) mountTreeItem);
#endif
	uio_free(mountTreeItem);
}


// *** uio_MountInfo *** //

uio_MountInfo *
uio_MountInfo_new(uio_FileSystemID fsID, uio_MountTree *mountTree,
		uio_PDirHandle *pDirHandle, char *dirName, uio_AutoMount **autoMount,
		uio_MountHandle *mountHandle, int flags) {
	uio_MountInfo *result;
	
	result = uio_MountInfo_alloc();
	result->fsID = fsID;
	result->mountTree = mountTree;
	result->pDirHandle = pDirHandle;
	result->dirName = dirName;
	result->autoMount = autoMount;
	result->mountHandle = mountHandle;
	result->flags = flags;
	return result;
}

void
uio_MountInfo_delete(uio_MountInfo *mountInfo) {
	uio_free(mountInfo->dirName);
	uio_PDirHandle_unref(mountInfo->pDirHandle);
	uio_MountInfo_free(mountInfo);
}

static inline uio_MountInfo *
uio_MountInfo_alloc(void) {
	uio_MountInfo *result = uio_malloc(sizeof (uio_MountInfo));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_MountInfo, (void *) result);
#endif
	return result;
}

static inline void
uio_MountInfo_free(uio_MountInfo *mountInfo) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_MountInfo, (void *) mountInfo);
#endif
	uio_free(mountInfo);
}


