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

#ifndef LIBS_UIO_MATCH_H_
#define LIBS_UIO_MATCH_H_

typedef struct match_MatchContext match_MatchContext;

#include <sys/types.h>

// TODO: make this into a configurable option
//#define HAVE_GLOB
#define HAVE_REGEX


typedef enum {
	match_MATCH_LITERAL = 0,
	match_MATCH_PREFIX,
	match_MATCH_SUFFIX,
	match_MATCH_SUBSTRING,
#ifdef HAVE_GLOB
	match_MATCH_GLOB,
#endif
#ifdef HAVE_REGEX
	match_MATCH_REGEX,
#endif
} match_MatchType;

typedef int match_Result;
#define match_NOMATCH    0
#define match_MATCH      1
#define match_OK         0	
#define match_EUNKNOWN  -1
#define match_ENOSYS    -2
#define match_ECUSTOM   -3
#define match_ENOTINIT  -4

typedef struct match_LiteralContext match_LiteralContext;
typedef struct match_PrefixContext match_PrefixContext;
typedef struct match_SuffixContext match_SuffixContext;
typedef struct match_SubStringContext match_SubStringContext;
#ifdef HAVE_GLOB
typedef struct match_GlobContext match_GlobContext;
#endif
#ifdef HAVE_REGEX
typedef struct match_RegexContext match_RegexContext;
#endif

match_Result match_prepareContext(const char *pattern,
		match_MatchContext **contextPtr, match_MatchType type);
match_Result match_matchPattern(match_MatchContext *context,
		const char *string);
const char *match_errorString(match_MatchContext *context,
		match_Result result);
void match_freeContext(match_MatchContext *context);
match_Result match_matchPatternOnce(const char *pattern, match_MatchType type,
		const char *string);


/* *** Internal definitions follow *** */
#ifdef match_INTERNAL

#include <sys/types.h>
#ifdef HAVE_REGEX
#	include <regex.h>
#endif

#include "uioport.h"

struct match_MatchContext {
	match_MatchType type;
	union {
		match_LiteralContext *literal;
		match_PrefixContext *prefix;
		match_SuffixContext *suffix;
		match_SubStringContext *subString;
#ifdef HAVE_GLOB
		match_GlobContext *glob;
#endif
#ifdef HAVE_REGEX
		match_RegexContext *regex;
#endif
	} u;
};

struct match_LiteralContext {
	char *pattern;
};

struct match_PrefixContext {
	char *pattern;
};

struct match_SuffixContext {
	char *pattern;
	size_t len;
			// for speed
};

struct match_SubStringContext {
	char *pattern;
};

#ifdef HAVE_GLOB
struct match_GlobContext {
	char *pattern;
};
#endif

#ifdef HAVE_REGEX
struct match_RegexContext {
	regex_t native;
	char *errorString;
	int errorCode;
	int flags;
#define match_REGEX_INITIALISED 1
};
#endif

match_Result match_prepareLiteral(const char *pattern,
		match_LiteralContext **contextPtr);
match_Result match_matchLiteral(match_LiteralContext *context,
		const char *string);
void match_freeLiteral(match_LiteralContext *context);

match_Result match_preparePrefix(const char *pattern,
		match_PrefixContext **contextPtr);
match_Result match_matchPrefix(match_PrefixContext *context,
		const char *string);
void match_freePrefix(match_PrefixContext *context);

match_Result match_prepareSuffix(const char *pattern,
		match_SuffixContext **contextPtr);
match_Result match_matchSuffix(match_SuffixContext *context,
		const char *string);
void match_freeSuffix(match_SuffixContext *context);

match_Result match_prepareSubString(const char *pattern,
		match_SubStringContext **contextPtr);
match_Result match_matchSubString(match_SubStringContext *context,
		const char *string);
void match_freeSubString(match_SubStringContext *context);

#ifdef HAVE_GLOB
match_Result match_prepareGlob(const char *pattern,
		match_GlobContext **contextPtr);
match_Result match_matchGlob(match_GlobContext *context,
		const char *string);
void match_freeGlob(match_GlobContext *context);
#endif  /* HAVE_GLOB */

#ifdef HAVE_REGEX
match_Result match_prepareRegex(const char *pattern,
		match_RegexContext **contextPtr);
match_Result match_matchRegex(match_RegexContext *context,
		const char *string);
const char *match_errorStringRegex(match_RegexContext *context,
		int errorCode);
void match_freeRegex(match_RegexContext *context);
#endif

#endif  /* match_INTERNAL */

#endif  /* LIBS_UIO_MATCH_H_ */

