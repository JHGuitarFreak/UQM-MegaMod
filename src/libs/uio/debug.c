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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#if defined(__unix__) && !defined(_WIN32_WCE)
#	include <sys/wait.h>
#endif

#include "debug.h"
#include "uioport.h"
#include "io.h"
#include "utils.h"
#include "types.h"
#include "mem.h"
#include "uioutils.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

#define LINEBUFLEN 1024

typedef struct DebugContext {
	uio_bool exit;
	FILE *in;
	FILE *out;
	FILE *err;
	uio_DirHandle *cwd;
} DebugContext;

typedef struct {
	const char *name;
	int (*fun)(DebugContext *, int, char *[]);
} DebugCommand;

#ifdef STAND_ALONE
void initRepository(void);
void unInitRepository(void);
#endif

static int debugCmdCat(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdCd(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdExec(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdExit(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdLs(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdMem(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdMkDir(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdMount(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdMv(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdPwd(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdRm(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdRmDir(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdStat(DebugContext *debugContext, int argc, char *argv[]);
static int debugCmdWriteTest(DebugContext *debugContext, int argc,
		char *argv[]);
static int debugCmdFwriteTest(DebugContext *debugContext, int argc,
		char *argv[]);

static void makeArgs(char *lineBuf, int *argc, char ***argv);
static int debugCallCommand(DebugContext *debugContext,
		int argc, char *argv[]);
uio_MountHandle *
debugMountOne(uio_Repository *destRep, const char *mountPoint,
		uio_FileSystemID fsType,
		uio_DirHandle *sourceDir, const char *sourcePath,
		const char *inPath, uio_AutoMount **autoMount, int flags,
		uio_MountHandle *relative);

// In alphabetic order:
DebugCommand debugCommands[] = {
	{ "cat", debugCmdCat },
	{ "cd", debugCmdCd },
	{ "exec", debugCmdExec },
	{ "exit", debugCmdExit },
	{ "fwritetest", debugCmdFwriteTest },
	{ "ls", debugCmdLs },
	{ "mem", debugCmdMem },
	{ "mkdir", debugCmdMkDir },
	{ "mount", debugCmdMount },
	{ "mv", debugCmdMv },
	{ "pwd", debugCmdPwd },
	{ "rm", debugCmdRm },
	{ "rmdir", debugCmdRmDir },
	{ "stat", debugCmdStat },
	{ "writetest", debugCmdWriteTest },
};


#ifndef STAND_ALONE
extern uio_Repository *repository;
#else
uio_Repository *repository;
uio_MountHandle *mountHandles[9];  // TODO: remove (this is just a test)

int
main(int argc, char *argv[]) {
	initRepository();
	uio_debugInteractive(stdin, stdout, stderr);
	unInitRepository();
	(void) argc;
	(void) argv;
	return EXIT_SUCCESS;
}

uio_MountHandle *
debugMountOne(uio_Repository *destRep, const char *mountPoint,
		uio_FileSystemID fsType,
		uio_DirHandle *sourceDir, const char *sourcePath,
		const char *inPath, uio_AutoMount **autoMount, int flags,
		uio_MountHandle *relative) {
	uio_MountHandle *mountHandle;

	mountHandle = uio_mountDir(destRep, mountPoint, fsType, sourceDir,
			sourcePath, inPath, autoMount, flags, relative);
	if (mountHandle == NULL) {
		int savedErrno = errno;
		fprintf(stderr, "Could not mount '%s' and graft '%s' from that "
				"into the repository at '%s': %s\n",
				sourcePath, inPath, mountPoint, strerror(errno));
		errno = savedErrno;
	}
	return mountHandle;
}

void
initRepository(void) {
	static uio_AutoMount autoMountZip = {
		.pattern =      "*.zip",
		.matchType =    match_MATCH_SUFFIX,
		.fileSystemID = uio_FSTYPE_ZIP,
		.mountFlags =   uio_MOUNT_BELOW | uio_MOUNT_RDONLY
	};
	static uio_AutoMount *autoMount[] = {
		&autoMountZip,
		NULL
	};

	uio_init();
	repository = uio_openRepository(0);

	memset(&mountHandles, '\0', sizeof mountHandles);
#if 1
	mountHandles[0] = debugMountOne(repository, "/", uio_FSTYPE_STDIO,
			NULL, NULL, "/home/svdb/cvs/sc2/content", autoMount,
			uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif
#if 1
	mountHandles[1] = debugMountOne(repository, "/", uio_FSTYPE_STDIO,
			NULL, NULL, "/home/svdb/cvs/sc2/src/sc2code/ships",
			autoMount, uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif
#if 1
	mountHandles[2] = debugMountOne(repository, "/", uio_FSTYPE_STDIO,
			NULL, NULL, "/tmp/vfstest", autoMount, uio_MOUNT_TOP, NULL);
#endif
#if 1
	mountHandles[3] = debugMountOne(repository, "/", uio_FSTYPE_STDIO,
			NULL, NULL, "/tmp/vfstest2", autoMount, uio_MOUNT_TOP, NULL);
#endif

	// TODO: should work too:
#if 0
	mountHandle[4] = debugMountOne(repository, "/zip/", uio_FSTYPE_ZIP, NULL,
			NULL, "/ziptest/foo.zip", autoMount, uio_MOUNT_TOP, NULL);
#endif
	{
		uio_DirHandle *rootDir;
		rootDir = uio_openDir(repository, "/", 0);
		if (rootDir == NULL) {
			fprintf(stderr, "Could not open '/' dir.\n");
		} else {
#if 1
			mountHandles[4] = debugMountOne(repository, "/example/",
					uio_FSTYPE_ZIP, rootDir, "/example2.zip", "/", autoMount,
					uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif
#if 1
			mountHandles[5] = debugMountOne(repository, "/example/",
					uio_FSTYPE_ZIP, rootDir, "/example/example.zip", "/",
					autoMount, uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif
#if 1
			mountHandles[6] = debugMountOne(repository, "/zip/",
					uio_FSTYPE_ZIP, rootDir, "/voice.zip", "/", autoMount,
					uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif
#if 1
			mountHandles[7] = debugMountOne(repository, "/foo/",
					uio_FSTYPE_ZIP, rootDir, "/foo2.zip", "/", autoMount,
					uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif
			uio_closeDir(rootDir);
		}
	}
	mountHandles[8] = debugMountOne(repository, "/tmp/",
			uio_FSTYPE_STDIO, NULL, NULL, "/tmp/", autoMount,
			uio_MOUNT_TOP, NULL);

#if 1
	mountHandles[8] = debugMountOne(repository, "/root/root/",
			uio_FSTYPE_STDIO, NULL, NULL, "/", autoMount,
			uio_MOUNT_TOP | uio_MOUNT_RDONLY, NULL);
#endif

}

void
unInitRepository(void) {
#if 1
	int i;
//	uio_printMountTree(stderr, repository->mountTree, 0);
//	fprintf(stderr, "\n");
	for (i = 7; i >= 0; i--) {
		if (mountHandles[i] != NULL)
			uio_unmountDir(mountHandles[i]);
//		uio_printMountTree(stderr, repository->mountTree, 0);
//		uio_printMounts(stderr, repository);
//		fprintf(stderr, "\n");
	}
#endif
	uio_closeRepository(repository);
	uio_unInit();
}
#endif  /* STAND_ALONE */

void
uio_debugInteractive(FILE *in, FILE *out, FILE *err) {
	char lineBuf[LINEBUFLEN];
	size_t lineLen;
	int argc;
	char **argv;
	DebugContext debugContext;
	uio_bool interactive;
	
	memset(&debugContext, '\0', sizeof (DebugContext));
	debugContext.exit = false;
	debugContext.in = in;
	debugContext.out = out;
	debugContext.err = err;
	debugContext.cwd = uio_openDir(repository, "/", 0);
	if (debugContext.cwd == NULL) {
		fprintf(err, "Fatal: Could not open working dir.\n");
		abort();
	}
	
	interactive = isatty(fileno(in));
	do {
		if (interactive)
			fprintf(out, "> ");
		if (fgets(lineBuf, LINEBUFLEN, in) == NULL) {
			if (feof(in)) {
				// user pressed ^D
				break;
			}
			// error occured
			clearerr(in);
			continue;
		}
		lineLen = strlen(lineBuf);
		if (lineBuf[lineLen - 1] != '\n' && lineBuf[lineLen - 1] != '\r') {
			fprintf(err, "Too long command line.\n");
			// TODO: read until EOL
			continue;
		}
		makeArgs(lineBuf, &argc, &argv);
		if (argc == 0) {
			uio_free(argv);
			continue;
		}
		debugCallCommand(&debugContext, argc, argv);
		uio_free(argv);
	} while (!debugContext.exit);
	if (interactive)
		fprintf(out, "\n");
	uio_closeDir(debugContext.cwd);
}

static void
makeArgs(char *lineBuf, int *argc, char ***argv) {
	int numArg;
	char **args;
	char *ptr;

	numArg = 0;
	ptr = lineBuf;
	while(true) {
		while (isspace((int) *ptr))
			ptr++;
		if (*ptr == '\0')
			break;
		numArg++;
		while (!isspace((int) *ptr))
			ptr++;
	}

	args = uio_malloc((numArg + 1) * sizeof (char *));
	numArg = 0;
	ptr = lineBuf;
	while(true) {
		while (isspace((int) *ptr))
			ptr++;
		if (*ptr == '\0')
			break;
		args[numArg] = ptr;
		numArg++;
		while (!isspace((int) *ptr))
			ptr++;
		if (*ptr == '\0')
			break;
		*ptr = '\0';
		ptr++;
	}
	args[numArg] = NULL;
	*argv = args;
	*argc = numArg;
}

static int
debugCallCommand(DebugContext *debugContext, int argc, char *argv[]) {
	int i;
	int numDebugCommands;
	
	i = 0;
	numDebugCommands = sizeof debugCommands / sizeof debugCommands[0];
	// could be improved with binary search
	while(1) {
		if (i == numDebugCommands) {
			fprintf(debugContext->err, "Invalid command.\n");
			return 1;
		}
		if (strcmp(argv[0], debugCommands[i].name) == 0)
			break;		
		i++;
	}
	return debugCommands[i].fun(debugContext, argc, argv);
}

static int
debugCmdCat(DebugContext *debugContext, int argc, char *argv[]) {
	uio_Handle *handle;
#define READBUFSIZE 0x10000
	char readBuf[READBUFSIZE];
	char *bufPtr;
	ssize_t numInBuf, numWritten;
	size_t totalWritten;

	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	handle = uio_open(debugContext->cwd, argv[1], O_RDONLY
#ifdef WIN32
			| O_BINARY
#endif
			, 0);
	if (handle == NULL) {
		fprintf(debugContext->err, "Could not open file: %s\n",
				strerror(errno));
		return 1;
	}

	totalWritten = 0;
	while (1) {
		numInBuf = uio_read(handle, readBuf, READBUFSIZE);
		if (numInBuf == -1) {
			if (errno == EINTR)
				continue;
			fprintf(debugContext->err, "Could not read from file: %s\n",
					strerror(errno));
			uio_close(handle);
			return 1;
		}
		if (numInBuf == 0)
			break;
		bufPtr = readBuf;
		do {
			numWritten = write(fileno(debugContext->out), bufPtr, numInBuf);
			if (numWritten == -1)
			{
				if (errno == EINTR)
					continue;
				fprintf(debugContext->err, "Could not read from file: %s\n",
						strerror(errno));
				uio_close(handle);
			}
			numInBuf -= numWritten;
			bufPtr += numWritten;
			totalWritten += numWritten;
		} while (numInBuf > 0);
	}
	fprintf(debugContext->out, "[%u bytes]\n", (unsigned int) totalWritten);
	
	uio_close(handle);
	return 0;
}

static int
debugCmdCd(DebugContext *debugContext, int argc, char *argv[]) {
	uio_DirHandle *newWd;

	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	newWd = uio_openDirRelative(debugContext->cwd, argv[1], 0);
	if (newWd == NULL) {
		fprintf(debugContext->err, "Could not access new dir: %s\n",
				strerror(errno));
		return 1;
	}
	uio_closeDir(debugContext->cwd);
	debugContext->cwd = newWd;
	return 0;
}

static int
debugCmdExec(DebugContext *debugContext, int argc, char *argv[]) {
	int i;
	const char **newArgs;
	int errCode = 0;
	uio_StdioAccessHandle **handles;
	uio_DirHandle *tempDir;

	if (argc < 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}

	tempDir = uio_openDirRelative(debugContext->cwd, "/tmp", 0);
	if (tempDir == 0) {
		fprintf(debugContext->err, "Could not open temp dir: %s.\n",
				strerror(errno));
		return 1;
	}

	newArgs = uio_malloc(argc * sizeof (char *));
	newArgs[0] = argv[1];
	handles = uio_malloc(argc * sizeof (uio_StdioAccessHandle *));
	handles[0] = NULL;
	
	for (i = 2; i < argc; i++) {
#if 0
		if (argv[i][0] == '-') {
			// Don't try to parse arguments that start with '-'.
			// They are probably option flags.
			newArgs[i - 1] = argv[i];
		}
#endif
		handles[i - 1] = uio_getStdioAccess(debugContext->cwd, argv[i],
				O_RDONLY, tempDir);
		if (handles[i - 1] == NULL) {
			if (errno == ENOENT) {
				// No match; we keep what's typed literally.
				newArgs[i - 1] = argv[i];
				continue;
			}

			// error
			fprintf(debugContext->err,
					"Cannot execute: Cannot get stdio access to %s: %s.\n",
					argv[i], strerror(errno));
			errCode = 1;
			argc = i + 1;
			goto err;
		}

		newArgs[i - 1] = uio_StdioAccessHandle_getPath(handles[i - 1]);
	}
	newArgs[argc - 1] = NULL;

	fprintf(debugContext->err, "Executing: %s", newArgs[0]);
	for (i = 1; i < argc - 1; i++)
		fprintf(debugContext->err, " %s", newArgs[i]);
	fprintf(debugContext->err, "\n");

#if defined(__unix__) && !defined(_WIN32_WCE)
	{
		pid_t pid;
		
		pid = fork();
		switch (pid) {
			case -1:
				fprintf(debugContext->err, "Error: fork() failed: %s.\n",
						strerror(errno));
				break;
			case 0:
				// child
				execvp(newArgs[0], (char * const *) newArgs);
				fprintf(debugContext->err, "Error: execvp() failed: %s.\n",
						strerror(errno));
				_exit(EXIT_FAILURE);
				break;
			default: {
				// parent
				int status;
				pid_t retVal;

				while (1) {
					retVal = waitpid(pid, &status, 0);
					if (retVal != -1)
						break;
					if (errno != EINTR) {
						fprintf(debugContext->err, "Error: waitpid() "
								"failed: %s\n", strerror(errno));
						break;
					}
				}
				if (retVal == -1)
					break;

				if (WIFEXITED(status)) {
					fprintf(debugContext->err, "Exit status: %d\n",
							WEXITSTATUS(status));
				} else if (WIFSIGNALED(status)) {
					fprintf(debugContext->err, "Terminated on signal %d.\n",
							WTERMSIG(status));
				} else {
					fprintf(debugContext->err, "Error: weird exit status.\n");
				}
				break;
			}
		}
	}
#else
	fprintf(debugContext->err, "Cannot execute: not supported on this "
			"platform.\n");
#endif

err:
	for (i = 1; i < argc - 1; i++) {
		if (handles[i] != NULL)
			uio_releaseStdioAccess(handles[i]);
	}
	uio_free(handles);
	uio_free((void *) newArgs);
	uio_closeDir(tempDir);

	return errCode;
}

static int
debugCmdExit(DebugContext *debugContext, int argc, char *argv[]) {
	debugContext->exit = true;
	(void) argc;
	(void) argv;
	return 0;
}

static int
debugCmdFwriteTest(DebugContext *debugContext, int argc, char *argv[]) {
	uio_Stream *stream;
	const char testString[] = "0123456789\n";

	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}

	stream = uio_fopen(debugContext->cwd, argv[1], "w+b");
	if (stream == NULL) {
		fprintf(debugContext->err, "Could not open file: %s\n",
				strerror(errno));
		goto err;
	}

	if (uio_fwrite(testString, strlen(testString), 1, stream) != 1) {
		fprintf(debugContext->err, "uio_fwrite() failed: %s\n",
				strerror(errno));
		goto err;
	}
	if (uio_fputs(testString, stream) == EOF) {
		fprintf(debugContext->err, "uio_fputs() failed: %s\n",
				strerror(errno));
		goto err;
	}
	if (uio_fseek(stream, 15, SEEK_SET) != 0) {
		fprintf(debugContext->err, "uio_fseek() failed: %s\n",
				strerror(errno));
		goto err;
	}
	if (uio_fputc('A', stream) != 'A') {
		fprintf(debugContext->err, "uio_fputc() failed: %s\n",
				strerror(errno));
		goto err;
	}
	if (uio_fseek(stream, 0, SEEK_SET) != 0) {
		fprintf(debugContext->err, "uio_fseek() failed: %s\n",
				strerror(errno));
		goto err;
	}
	{
		char buf[6];
		char *ptr;
		int i;
		i = 1;
		while (1) {
			ptr = uio_fgets(buf, sizeof buf, stream);
			if (ptr == NULL)
				break;
			fprintf(debugContext->out, "%d: [%s]\n", i, ptr);
			i++;
		}
		if (uio_ferror(stream)) {
			fprintf(debugContext->err, "uio_fgets() failed: %s\n",
					strerror(errno));
			goto err;
		}
		uio_clearerr(stream);
	}
	if (uio_fseek(stream, 4, SEEK_END) != 0) {
		fprintf(debugContext->err, "uio_fseek() failed: %s\n",
				strerror(errno));
		goto err;
	}
	{
		char buf[2000];
		memset(buf, 'Q', sizeof buf);
		if (uio_fwrite(buf, 100, 20, stream) != 20) {
			fprintf(debugContext->err, "uio_fwrite() failed: %s\n",
					strerror(errno));
			goto err;
		}
	}
	if (uio_fseek(stream, 5, SEEK_SET) != 0) {
		fprintf(debugContext->err, "uio_fseek() failed: %s\n",
				strerror(errno));
		goto err;
	}
	if (uio_fputc('B', stream) != 'B') {
		fprintf(debugContext->err, "uio_fputc() failed: %s\n",
				strerror(errno));
		goto err;
	}
	uio_fclose(stream);
	return 0;
err:
	uio_fclose(stream);
	return 1;
}

static int
listOneDir(DebugContext *debugContext, const char *arg) {
	uio_DirList *dirList;
	int i;
	const char *pattern;
	const char *cpath;
	char *buf = NULL;

	if (arg[0] == '\0') {
		cpath = arg;
		pattern = "*";
	} else {
		pattern = strrchr(arg, '/');
		if (pattern == NULL) {
			// No directory component in 'arg'.
			cpath = "";
			pattern = arg;
		} else if (pattern[1] == '\0') {
			// 'arg' ends on /
			cpath = arg;
			pattern = "*";
		} else {
			if (pattern == arg) {
				cpath = "/";
			} else {
				buf = uio_malloc(pattern - arg + 1);
				memcpy(buf, arg, pattern - arg);
				buf[pattern - arg] = '\0';
				cpath = buf;
			}
			pattern++;
		}
	}
#ifdef HAVE_GLOB
	dirList = uio_getDirList(debugContext->cwd, cpath, pattern,
			match_MATCH_GLOB);
#else
	if (pattern[0] == '*' && pattern[1] == '\0') {
		dirList = uio_getDirList(debugContext->cwd, cpath, "",
				match_MATCH_PREFIX);
	} else {
		dirList = uio_getDirList(debugContext->cwd, cpath, pattern,
				match_MATCH_LITERAL);
	}
#endif
	if (dirList == NULL) {
		fprintf(debugContext->out, "Error in uio_getDirList(): %s.\n",
				strerror(errno));
		if (buf != NULL)
			uio_free(buf);
		return 1;
	}
	for (i = 0; i < dirList->numNames; i++)
		fprintf(debugContext->out, "%s\n", dirList->names[i]);
	uio_DirList_free(dirList);
	if (buf != NULL)
		uio_free(buf);
	return 0;
}

static int
debugCmdLs(DebugContext *debugContext, int argc, char *argv[]) {
	int argI;
	int retVal;

	if (argc == 1)
		return listOneDir(debugContext, unconst(""));
	
	for (argI = 1; argI < argc; argI++) {
		retVal = listOneDir(debugContext, argv[argI]);
		if (retVal != 0)
			return retVal;
	}
	
	return 0;
}

static int
debugCmdMem(DebugContext *debugContext, int argc, char *argv[]) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_printPointers(debugContext->out);
#else
	fprintf(debugContext->out, "Memory debugging not compiled in.\n");
#endif
	(void) argc;
	(void) argv;
	return 0;
}

static int
debugCmdMkDir(DebugContext *debugContext, int argc, char *argv[]) {
	int retVal;
	
	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	retVal = uio_mkdir(debugContext->cwd, argv[1], 0777);
	if (retVal == -1) {
		fprintf(debugContext->err, "Could not create directory: %s\n",
				strerror(errno));
		return 1;
	}
	return 0;
}

static int
debugCmdMount(DebugContext *debugContext, int argc, char *argv[]) {
	if (argc == 1) {
		uio_printMounts(debugContext->out, repository);
//		uio_printMountTree(debugContext->out, repository->mountTree, 0);
	}
	(void) argv;
	return 0;
}

static int
debugCmdMv(DebugContext *debugContext, int argc, char *argv[]) {
	int retVal;
	
	if (argc != 3) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	retVal = uio_rename(debugContext->cwd, argv[1],
			debugContext->cwd, argv[2]);
	if (retVal == -1) {
		fprintf(debugContext->err, "Could not rename: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

static int
debugCmdPwd(DebugContext *debugContext, int argc, char *argv[]) {
	uio_DirHandle_print(debugContext->cwd, debugContext->out);
	(void) argc;
	(void) argv;
	return 0;
}

static int
debugCmdRm(DebugContext *debugContext, int argc, char *argv[]) {
	int retVal;
	
	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	retVal = uio_unlink(debugContext->cwd, argv[1]);
	if (retVal == -1) {
		fprintf(debugContext->err, "Could not remove file: %s\n",
				strerror(errno));
		return 1;
	}
	return 0;
}

static int
debugCmdRmDir(DebugContext *debugContext, int argc, char *argv[]) {
	int retVal;
	
	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	retVal = uio_rmdir(debugContext->cwd, argv[1]);
	if (retVal == -1) {
		fprintf(debugContext->err, "Could not remove directory: %s\n",
				strerror(errno));
		return 1;
	}
	return 0;
}

static int
debugCmdStat(DebugContext *debugContext, int argc, char *argv[]) {
	struct stat statBuf;

	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	if (uio_stat(debugContext->cwd, argv[1], &statBuf) == -1) {
		// errno is set
		int savedErrno;
		savedErrno = errno;
		fprintf(debugContext->err, "Could not stat file: %s\n",
				strerror(errno));
		errno = savedErrno;
		return 1;
	}

	fprintf(debugContext->out,
			"size %ld bytes\n"
			"uid %d  gid %d  mode 0%o\n",
			(unsigned long) statBuf.st_size,
			(unsigned int) statBuf.st_uid,
			(unsigned int) statBuf.st_gid,
			(unsigned int) statBuf.st_mode &
			(S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID));
	// Can't do these next three in one fprintf, as ctime uses a static buffer
	// that is overwritten with each call.
	fprintf(debugContext->out,
			"last access:        %s", ctime(&statBuf.st_atime));
	fprintf(debugContext->out,
			"last modification:  %s", ctime(&statBuf.st_mtime));
	fprintf(debugContext->out,
			"last status change: %s", ctime(&statBuf.st_ctime));
	
	return 0;
}

static int
debugCmdWriteTest(DebugContext *debugContext, int argc, char *argv[]) {
	uio_Handle *handle;
	const char testString[] = "Hello world!\n";

	if (argc != 2) {
		fprintf(debugContext->err, "Invalid number of arguments.\n");
		return 1;
	}
	
	handle = uio_open(debugContext->cwd, argv[1],
			O_WRONLY | O_CREAT | O_EXCL
#ifdef WIN32
			| O_BINARY
#endif
			, 0644);
	if (handle == NULL) {
		fprintf(debugContext->err, "Could not open file: %s\n",
				strerror(errno));
		return 1;
	}

	if (uio_write(handle, testString, sizeof testString) == -1) {
		fprintf(debugContext->err, "Write failed: %s\n",
				strerror(errno));
		return 1;
	}
	
	uio_close(handle);
	return 0;
}


