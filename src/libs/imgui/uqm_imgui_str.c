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

#include "uqm_imgui.h"

typedef struct
{
	char *id;
	char **values;
	int value_count;
} ImguiString;

typedef struct
{
	ImguiString *strings;
	int count;
} ImguiStringTable;

typedef struct
{
	const char *id;
	const char **cached_array;
	int cached_count;
	int max_cached_count;
} ImguiStringCache;

static ImguiStringTable imgui_strings = { NULL, 0 };
static ImguiStringCache *string_cache = NULL;
static int cache_count = 0;
static int cache_capacity = 0;

static const char **
GetCachedStringArray (const char *id, int *count, int max_count)
{
	int i;

	for (i = 0; i < cache_count; i++)
	{
		if (strcmp (string_cache[i].id, id) == 0)
		{
			*count = string_cache[i].cached_count;
			return string_cache[i].cached_array;
		}
	}

	return NULL;
}

static void
CacheStringArray (const char *id, const char **array,
		int count, int max_count)
{
	ImguiStringCache *cache;
	int i;

	if (cache_count >= cache_capacity)
	{
		cache_capacity = cache_capacity ? cache_capacity * 2 : 16;
		string_cache = realloc (string_cache,
				cache_capacity * sizeof (ImguiStringCache));
	}

	cache = &string_cache[cache_count];
	cache->id = strdup (id);
	cache->cached_count = count;
	cache->max_cached_count = max_count;

	cache->cached_array = malloc (max_count * sizeof (const char *));
	for (i = 0; i < count && i < max_count; i++)
	{
		cache->cached_array[i] = array[i];
	}

	cache_count++;
}

static char **
parse_csv_line (char *line, int *token_count)
{
	char **tokens = NULL;
	int max_tokens = 10;
	char *ptr, *start;
	int in_quotes, len;

	*token_count = 0;

	tokens = malloc (max_tokens * sizeof (char *));
	if (!tokens) return NULL;

	ptr = line;
	start = line;
	in_quotes = 0;

	while (*ptr)
	{
		if (*ptr == '"' && (ptr == line || *(ptr - 1) != '\\'))
		{
			in_quotes = !in_quotes;
		}
		else if (*ptr == ',' && !in_quotes)
		{
			len = ptr - start;
			tokens[*token_count] = malloc (len + 1);
			strncpy (tokens[*token_count], start, len);
			tokens[*token_count][len] = '\0';

			(*token_count)++;
			start = ptr + 1;

			if (*token_count >= max_tokens)
			{
				max_tokens *= 2;
				tokens = realloc (tokens, max_tokens * sizeof (char *));
			}
		}
		ptr++;
	}

	if (*start)
	{
		len = ptr - start;
		tokens[*token_count] = malloc (len + 1);
		strncpy (tokens[*token_count], start, len);
		tokens[*token_count][len] = '\0';
		(*token_count)++;
	}

	return tokens;
}

static void
LoadImguiStrings (void)
{
	char *csv_path;
	int len, line_num, i;
	size_t base_len;
	const char *slash;
	FILE *f;
	char line[1024];

	if (imgui_strings.strings) return;

	base_len = strlen (baseContentPath);
	if (base_len > 0)
	{
		char last_char = baseContentPath[base_len - 1];
		slash = (last_char == '/' || last_char == '\\') ? "" : "/";
	}
	else
		slash = "/";

	len = snprintf (NULL, 0, "%s%simgui.csv",
		baseContentPath, slash);

	csv_path = HMalloc (len + 1);

	snprintf (csv_path, len + 1, "%s%simgui.csv",
		baseContentPath, slash);

	f = fopen (csv_path, "r");
	if (!f)
	{
		log_add (log_Warning, "Could not open ImGui strings: %s", csv_path);

		HFree (csv_path);
		return;
	}

	HFree (csv_path);

	line_num = 0;

	while (fgets (line, sizeof (line), f))
	{
		char **tokens;
		int token_count, j;
		ImguiString *entry;

		line_num++;

		line[strcspn (line, "\r\n")] = 0;

		if (line[0] == '#' || line[0] == '\0')
			continue;

		tokens = parse_csv_line (line, &token_count);

		if (token_count < 2)
		{
			for (i = 0; i < token_count; i++)
				free (tokens[i]);
			free (tokens);
			continue;
		}

		imgui_strings.strings = realloc (imgui_strings.strings,
			(imgui_strings.count + 1) * sizeof (ImguiString));

		entry = &imgui_strings.strings[imgui_strings.count];

		entry->id = strdup (tokens[0]);
		entry->value_count = token_count - 1;
		entry->values = malloc (entry->value_count * sizeof (char *));

		for (j = 0; j < entry->value_count; j++)
		{
			entry->values[j] = strdup (tokens[j + 1]);
		}

		imgui_strings.count++;

		for (i = 0; i < token_count; i++)
			free (tokens[i]);
		free (tokens);
	}

	fclose (f);
	log_add (log_Info, "Loaded %d ImGui strings", imgui_strings.count);
}

const char *
Imgui_GetString (const char *id)
{
	int i;

	LoadImguiStrings ();

	for (i = 0; i < imgui_strings.count; i++)
	{
		if (strcmp (imgui_strings.strings[i].id, id) == 0)
		{
			return imgui_strings.strings[i].values[0];
		}
	}

	return id;
}

int
Imgui_GetStringArray (const char *id, const char **array, int max_count)
{
	int i, j, count;
	const char **cached;

	LoadImguiStrings ();

	count = 0;

	cached = GetCachedStringArray (id, &count, max_count);
	if (cached)
	{
		for (i = 0; i < count && i < max_count; i++)
		{
			array[i] = cached[i];
		}
		return count;
	}

	for (i = 0; i < imgui_strings.count; i++)
	{
		if (strcmp (imgui_strings.strings[i].id, id) == 0)
		{
			count = imgui_strings.strings[i].value_count;
			if (count > max_count) count = max_count;

			for (j = 0; j < count; j++)
			{
				array[j] = imgui_strings.strings[i].values[j];
			}

			CacheStringArray (id, array, count, max_count);

			return count;
		}
	}

	return 0;
}

static void
FreeImguiStrings (void)
{
	int i, j;

	for (i = 0; i < cache_count; i++)
	{
		free ((void *)string_cache[i].id);
		free (string_cache[i].cached_array);
	}
	free (string_cache);
	string_cache = NULL;
	cache_count = 0;
	cache_capacity = 0;

	for (i = 0; i < imgui_strings.count; i++)
	{
		free (imgui_strings.strings[i].id);
		for (j = 0; j < imgui_strings.strings[i].value_count; j++)
		{
			free (imgui_strings.strings[i].values[j]);
		}
		free (imgui_strings.strings[i].values);
	}
	free (imgui_strings.strings);
	imgui_strings.strings = NULL;
	imgui_strings.count = 0;
}