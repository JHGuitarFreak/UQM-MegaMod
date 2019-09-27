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

#include <string.h>
#ifdef DEBUG
#	include <stdio.h>
#endif

#define match_INTERNAL

#include "match.h"
#include "mem.h"
#include "uioport.h"

#ifdef HAVE_GLOB
#	include <fnmatch.h>
#endif


static inline match_MatchContext *match_allocMatchContext(void);
static inline void match_freeMatchContext(match_MatchContext *context);

static inline match_LiteralContext *match_newLiteralContext(char *pattern);
static inline match_LiteralContext *match_allocLiteralContext(void);
static inline void match_freeLiteralContext(match_LiteralContext *context);
static inline match_PrefixContext *match_newPrefixContext(char *pattern);
static inline match_PrefixContext *match_allocPrefixContext(void);
static inline void match_freePrefixContext(match_PrefixContext *context);
static inline match_SuffixContext *match_newSuffixContext(
		char *pattern, size_t len);
static inline match_SuffixContext *match_allocSuffixContext(void);
static inline void match_freeSuffixContext(match_SuffixContext *context);
static inline match_SubStringContext *match_newSubStringContext(
		char *pattern);
static inline match_SubStringContext *match_allocSubStringContext(void);
static inline void match_freeSubStringContext(match_SubStringContext *context);
#ifdef HAVE_GLOB
static inline match_GlobContext *match_newGlobContext(char *pattern);
static inline match_GlobContext *match_allocGlobContext(void);
static inline void match_freeGlobContext(match_GlobContext *context);
#endif  /* HAVE_GLOB */
#ifdef HAVE_REGEX
static inline match_RegexContext *match_newRegexContext(void);
static inline match_RegexContext *match_allocRegexContext(void);
static inline void match_freeRegexContext(match_RegexContext *context);
#endif  /* HAVE_REGEX */


// *** General part ***

static inline match_MatchContext *
match_allocMatchContext(void) {
	return uio_malloc(sizeof (match_MatchContext));
}

static inline void
match_freeMatchContext(match_MatchContext *context) {
	uio_free(context);
}

// NB: Even if this function fails, *contextPtr contains a context
//     which needs to be freed.
match_Result
match_prepareContext(const char *pattern, match_MatchContext **contextPtr,
		match_MatchType type) {
	match_Result result;

	*contextPtr = match_allocMatchContext();
	(*contextPtr)->type = type;
	switch (type) {
		case match_MATCH_LITERAL:
			result = match_prepareLiteral(pattern,
					&(*contextPtr)->u.literal);
			break;
		case match_MATCH_PREFIX:
			result = match_preparePrefix(pattern, &(*contextPtr)->u.prefix);
			break;
		case match_MATCH_SUFFIX:
			result = match_prepareSuffix(pattern, &(*contextPtr)->u.suffix);
			break;
		case match_MATCH_SUBSTRING:
			result = match_prepareSubString(pattern,
					&(*contextPtr)->u.subString);
			break;
#ifdef HAVE_GLOB
		case match_MATCH_GLOB:
			result = match_prepareGlob(pattern, &(*contextPtr)->u.glob);
			break;
#endif
#ifdef HAVE_REGEX
		case match_MATCH_REGEX:
			result = match_prepareRegex(pattern, &(*contextPtr)->u.regex);
			break;
#endif
		default:
#ifdef DEBUG
			fprintf(stderr, "match_prepareContext called with unsupported "
					"type %d matching.\n", type);
#endif
			return match_ENOSYS;
	}
	return result;
}

match_Result
match_matchPattern(match_MatchContext *context, const char *string) {
	switch (context->type) {
		case match_MATCH_LITERAL:
			return match_matchLiteral(context->u.literal, string);
		case match_MATCH_PREFIX:
			return match_matchPrefix(context->u.prefix, string);
		case match_MATCH_SUFFIX:
			return match_matchSuffix(context->u.suffix, string);
		case match_MATCH_SUBSTRING:
			return match_matchSubString(context->u.subString, string);
#ifdef HAVE_GLOB
		case match_MATCH_GLOB:
			return match_matchGlob(context->u.glob, string);
#endif
#ifdef HAVE_REGEX
		case match_MATCH_REGEX:
			return match_matchRegex(context->u.regex, string);
#endif
		default:
			abort();
	}
}

// context may be NULL
const char *
match_errorString(match_MatchContext *context, match_Result result) {
	switch (result) {
		case match_OK:
		case match_MATCH:
//		case match_NOMATCH:  // same value as match_OK
			return "Success";
		case match_ENOSYS:
			return "Not implemented";
		case match_ENOTINIT:
			return "Uninitialised use";
		case match_ECUSTOM:
			// Depends on match type. Printed below.
			break;
		default:
			return "Unknown error";
	}
			
	if (context == NULL)
		return "Unknown match-type specific error.";
				// We can't be any more specific if no 'context' is supplied.

	switch (context->type) {
#if 0
		case match_MATCH_LITERAL:
			return match_errorStringLiteral(context->u.literal, result);
		case match_MATCH_PREFIX:
			return match_errorStringPrefix(context->u.prefix, result);
		case match_MATCH_SUFFIX:
			return match_errorStringSuffix(context->u.suffix, result);
		case match_MATCH_SUBSTRING:
			return match_errorStringSubString(context->u.subString, result);
#ifdef HAVE_GLOB
		case match_MATCH_GLOB:
			return match_errorStringGlob(context->u.glob, result);
#endif
#endif
#ifdef HAVE_REGEX
		case match_MATCH_REGEX:
			return match_errorStringRegex(context->u.regex, result);
#endif
		default:
			abort();
	}
}

void
match_freeContext(match_MatchContext *context) {
	switch (context->type) {
		case match_MATCH_LITERAL:
			match_freeLiteral(context->u.literal);
			break;
		case match_MATCH_PREFIX:
			match_freePrefix(context->u.prefix);
			break;
		case match_MATCH_SUFFIX:
			match_freeSuffix(context->u.suffix);
			break;
		case match_MATCH_SUBSTRING:
			match_freeSubString(context->u.subString);
			break;
#ifdef HAVE_GLOB
		case match_MATCH_GLOB:
			match_freeGlob(context->u.glob);
			break;
#endif
#ifdef HAVE_REGEX
		case match_MATCH_REGEX:
			match_freeRegex(context->u.regex);
			break;
#endif
		default:
			abort();
	}
	match_freeMatchContext(context);
}

match_Result
match_matchPatternOnce(const char *pattern, match_MatchType type,
		const char *string) {
	match_MatchContext *context;
	match_Result result;

	result = match_prepareContext(pattern, &context, type);
	if (result != match_OK)
		goto out;

	result = match_matchPattern(context, string);

out:
	match_freeContext(context);
	return result;
}


// *** Literal part ***

match_Result
match_prepareLiteral(const char *pattern,
		match_LiteralContext **contextPtr) {
	*contextPtr = match_newLiteralContext(uio_strdup(pattern));
	return match_OK;
}

match_Result
match_matchLiteral(match_LiteralContext *context, const char *string) {
	return (strcmp(context->pattern, string) == 0) ?
			match_MATCH : match_NOMATCH;
}

void
match_freeLiteral(match_LiteralContext *context) {
	uio_free(context->pattern);
	match_freeLiteralContext(context);
}

static inline match_LiteralContext *
match_newLiteralContext(char *pattern) {
	match_LiteralContext *result;

	result = match_allocLiteralContext();
	result->pattern = pattern;
	return result;
}

static inline match_LiteralContext *
match_allocLiteralContext(void) {
	return uio_malloc(sizeof (match_LiteralContext));
}

static inline void
match_freeLiteralContext(match_LiteralContext *context) {
	uio_free(context);
}


// *** Prefix part ***

match_Result
match_preparePrefix(const char *pattern,
		match_PrefixContext **contextPtr) {
	*contextPtr = match_newPrefixContext(uio_strdup(pattern));
	return match_OK;
}

match_Result
match_matchPrefix(match_PrefixContext *context, const char *string) {
	char *patPtr;

	patPtr = context->pattern;
	while (1) {
		if (*patPtr == '\0') {
			// prefix has completely matched
			return match_MATCH;
		}
		if (*string == '\0') {
			// no more string left, and still prefix to match
			return match_NOMATCH;
		}
		if (*patPtr != *string)
			return match_NOMATCH;
		patPtr++;
		string++;
	}
}

void
match_freePrefix(match_PrefixContext *context) {
	uio_free(context->pattern);
	match_freePrefixContext(context);
}

static inline match_PrefixContext *
match_newPrefixContext(char *pattern) {
	match_PrefixContext *result;

	result = match_allocPrefixContext();
	result->pattern = pattern;
	return result;
}

static inline match_PrefixContext *
match_allocPrefixContext(void) {
	return uio_malloc(sizeof (match_PrefixContext));
}

static inline void
match_freePrefixContext(match_PrefixContext *context) {
	uio_free(context);
}


// *** Suffix part ***

match_Result
match_prepareSuffix(const char *pattern,
		match_SuffixContext **contextPtr) {
	*contextPtr = match_newSuffixContext(
			uio_strdup(pattern), strlen(pattern));
	return match_OK;
}

match_Result
match_matchSuffix(match_SuffixContext *context, const char *string) {
	size_t stringLen;

	stringLen = strlen(string);
	if (stringLen < context->len) {
		// Supplied suffix is larger than string
		return match_NOMATCH;
	}

	return memcmp(string + stringLen - context->len, context->pattern,
			context->len) == 0 ? match_MATCH : match_NOMATCH;
}

void
match_freeSuffix(match_SuffixContext *context) {
	uio_free(context->pattern);
	match_freeSuffixContext(context);
}

static inline match_SuffixContext *
match_newSuffixContext(char *pattern, size_t len) {
	match_SuffixContext *result;

	result = match_allocSuffixContext();
	result->pattern = pattern;
	result->len = len;
	return result;
}

static inline match_SuffixContext *
match_allocSuffixContext(void) {
	return uio_malloc(sizeof (match_SuffixContext));
}

static inline void
match_freeSuffixContext(match_SuffixContext *context) {
	uio_free(context);
}


// *** SubString part ***

match_Result
match_prepareSubString(const char *pattern,
		match_SubStringContext **contextPtr) {
	*contextPtr = match_newSubStringContext(uio_strdup(pattern));
	return match_OK;
}

match_Result
match_matchSubString(match_SubStringContext *context, const char *string) {
	return strstr(string, context->pattern) != NULL;
}

void
match_freeSubString(match_SubStringContext *context) {
	uio_free(context->pattern);
	match_freeSubStringContext(context);
}

static inline match_SubStringContext *
match_newSubStringContext(char *pattern) {
	match_SubStringContext *result;

	result = match_allocSubStringContext();
	result->pattern = pattern;
	return result;
}

static inline match_SubStringContext *
match_allocSubStringContext(void) {
	return uio_malloc(sizeof (match_SubStringContext));
}

static inline void
match_freeSubStringContext(match_SubStringContext *context) {
	uio_free(context);
}


// *** Glob part ***

#ifdef HAVE_GLOB
match_Result
match_prepareGlob(const char *pattern, match_GlobContext **contextPtr) {
	*contextPtr = match_newGlobContext(uio_strdup(pattern));
	return match_OK;
}

match_Result
match_matchGlob(match_GlobContext *context, const char *string) {
	int retval;

	retval = fnmatch(context->pattern, string, 0);
	switch (retval) {
		case 0:
			return match_MATCH;
		case FNM_NOMATCH:
			return match_NOMATCH;
#if 0
		case FNM_NOSYS:
			return match_ENOSYS;
#endif
		default:
			return match_EUNKNOWN;
	}
}

void
match_freeGlob(match_GlobContext *context) {
	uio_free(context->pattern);
	match_freeGlobContext(context);
}

static inline match_GlobContext *
match_newGlobContext(char *pattern) {
	match_GlobContext *result;

	result = match_allocGlobContext();
	result->pattern = pattern;
	return result;
}

static inline match_GlobContext *
match_allocGlobContext(void) {
	return uio_malloc(sizeof (match_GlobContext));
}

static inline void
match_freeGlobContext(match_GlobContext *context) {
	uio_free(context);
}
#endif  /* HAVE_GLOB */


// *** Regex part ***

#ifdef HAVE_REGEX
match_Result
match_prepareRegex(const char *pattern, match_RegexContext **contextPtr) {
	*contextPtr = match_newRegexContext();
	(*contextPtr)->errorCode = regcomp(&(*contextPtr)->native, pattern,
			REG_EXTENDED | REG_NOSUB);
	if ((*contextPtr)->errorCode == 0) {
		(*contextPtr)->flags = match_REGEX_INITIALISED;
		return match_OK;
	}
	return match_ECUSTOM;
}

match_Result
match_matchRegex(match_RegexContext *context, const char *string) {
	int retval;
	
	if ((context->flags & match_REGEX_INITIALISED) != 
			match_REGEX_INITIALISED) {
		return match_ENOTINIT;
	}
	if (context->errorString) {
		uio_free(context->errorString);
		context->errorString = NULL;
	}
	retval = regexec(&context->native, string, 0, NULL, 0);
	switch (retval) {
		case 0:
			return match_MATCH;
		case REG_NOMATCH:
			return match_NOMATCH;
		default:
			context->errorCode = retval;
			return match_ECUSTOM;
	}
}

const char *
match_errorStringRegex(match_RegexContext *context, int errorCode) {
	size_t errorStringLength;

	if (context->errorString != NULL)
		uio_free(context->errorString);

	errorStringLength = regerror(context->errorCode, &context->native,
			NULL, 0);
	context->errorString = uio_malloc(errorStringLength);
	regerror(context->errorCode, &context->native,
			context->errorString, errorStringLength);
	
	(void) errorCode;
	return context->errorString;
}

void
match_freeRegex(match_RegexContext *context) {
	regfree(&context->native);
	if (context->errorString)
		uio_free(context->errorString);
	match_freeRegexContext(context);
}

static inline match_RegexContext *
match_newRegexContext(void) {
	match_RegexContext *result;
	result = match_allocRegexContext();
	result->errorString = NULL;
	result->errorCode = 0;
	result->flags = 0;
	return result;
}

static inline match_RegexContext *
match_allocRegexContext(void) {
	return uio_malloc(sizeof (match_RegexContext));
}

static inline void
match_freeRegexContext(match_RegexContext *context) {
	uio_free(context);
}
#endif  /* HAVE_REGEX */

