/*
 * DOS .fnt file extractor
 * By Alex Volkov (codepro@usa.net), 20050207
 *
 * Based on code by Serge van den Boom (svdb@stack.nl)
 *
 * The GPL applies (to this file).
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#if defined(WIN32) && defined(_MSC_VER)
#	include <getopt.h>
#	include <direct.h>
#	define makedir(d) _mkdir(d)
#	define inline __inline
#else
#	include <unistd.h>
#	define makedir(d)  mkdir(d, 0777)
#endif

#ifdef WIN32
#	ifdef _MSC_VER
#		define MKDIR(name, mode) ((void) mode, _mkdir(name))
#	else
#		define MKDIR(name, mode) ((void) mode, mkdir(name))
#	endif
#else
#	define MKDIR mkdir
#endif

#if defined(__GNUC__)
#	if !defined(WIN32) || !defined(__WIN32) || !defined(__WIN32__)
#		include <alloca.h>
#	endif
#	define inline __inline__
#endif
#include <png.h>

#define countof(a) ( sizeof(a)/sizeof(*a) )
#ifndef offsetof
#	define offsetof(s,m) ( (size_t)(&((s*)0)->m) )
#endif

#if defined(_MSC_VER)
#	define PACKED
#	pragma pack(push, 1)
#elif defined(__GNUC__)
#	define PACKED __attribute__((packed))
#endif

#define MAX_FONT_CHARS	0x60

typedef struct
{
	uint32_t PACKED magic;  // Always FFFFFFFF
	uint32_t PACKED magic_two;  // Always FFFFFFFF
	uint8_t  PACKED leading;
	uint8_t  PACKED maxascender;
	uint8_t  PACKED maxdescender;
	uint8_t  PACKED charspacing;
	uint8_t  PACKED kernamount;
	// 2 chars per byte; upper and lower nibble
	uint8_t  PACKED kerntab[MAX_FONT_CHARS];
	uint8_t  PACKED padding[2];
} header_t;

typedef struct
{
	size_t size;
	uint8_t *data;
	int xsize;
	int malloced;
	int w, h;
	int dy;
	int datah;
	int acth;
	int kerning;
} char_info_t;

typedef struct
{
	int height;
	int leading;
	int max_descend;
	int max_ascend;
	int max_height;
	int spacing;
	int kernamount;
	uint32_t data_ofs;
	int first_char;
	int num_chars;
	char_info_t *chars;
} index_header_t;

#if defined(_MSC_VER)
#	pragma pack(pop)
#endif

struct options
{
	char *infile;
	char *outdir;
	char *prefix;
	char *config;
	int print;
	int verbose;
	int usemax;
	int zeropos;
	int ishex;
};

int verbose_level = 0;
void verbose (int level, const char *fmt, ...);

index_header_t *readIndex (uint8_t *buf);
void freeIndex (index_header_t *);
void parse_arguments (int argc, char *argv[], struct options *opts);
void printIndex (const index_header_t *, const uint8_t *buf, FILE *out);
void readChars (index_header_t *, const uint8_t *buf);
void calcMaxHeight (index_header_t *);
void updateCharHeights (index_header_t *, int bDataH);
void writeFiles (const index_header_t *, const char *path,
		const char *prefix, int zeropos, int ishex, const char *cfg,
		const char *infile);

uint16_t get_16_le (uint16_t *val);
uint32_t get_32_le (uint32_t *val);
uint8_t get_quad (const uint8_t *table, int index);

int
main (int argc, char *argv[])
{
	FILE *in;
	size_t inlen;
	uint8_t *buf;
	index_header_t *h;
	struct options opts;
	char *prefix;

	parse_arguments (argc, argv, &opts);
	verbose_level = opts.verbose;

	in = fopen (opts.infile, "rb");
	if (!in)
	{
		verbose (1, "Error: Could not open file %s: %s\n",
				opts.infile, strerror (errno));
		return EXIT_FAILURE;
	}

	fseek (in, 0, SEEK_END);
	inlen = ftell (in);
	fseek (in, 0, SEEK_SET);

	buf = malloc (inlen);
	if (!buf)
	{
		verbose (1, "Out of memory reading file\n");
		return EXIT_FAILURE;
	}
	if (inlen != fread (buf, 1, inlen, in))
	{
		verbose (1, "Cannot read file '%s'\n", opts.infile);
		return EXIT_FAILURE;
	}
	fclose (in);

	h = readIndex (buf);
	if (opts.print)
		printIndex (h, buf, stdout);

	prefix = opts.prefix;
	if (!prefix)
		prefix = "";

	if (!opts.print)
	{
		size_t len;

		len = strlen (opts.outdir);
		if (opts.outdir[len - 1] == '/')
			opts.outdir[len - 1] = '\0';

		writeFiles (h, opts.outdir, prefix, opts.zeropos, opts.ishex, opts.config, opts.infile);
	}

	free (buf);

	return EXIT_SUCCESS;
}

void
verbose (int level, const char *fmt, ...)
{
	va_list args;

	if (verbose_level < level)
		return;

	va_start (args, fmt);
	vfprintf (stderr, fmt, args);
	va_end (args);
}

void
usage (void)
{
	fprintf (stderr,
			"unfont -p <infile>\n"
			"unfont [-m] [-x] [-0 #] [-v #] [-o <outdir>] [-n <prefix>] [-c <config>] <infile>\n"
			"Options:\n"
			"\t-p  print header and frame info\n"
			"\t-o  stuff pngs into <outdir>\n"
			"\t-n  name pngs <prefix><N>.png\n"
			"\t-m  char images will be of maximum seen height\n"
			"\t-0  make <N> 0-prepended by # amount\n"
			"\t-v  increase verbosity level by # amount\n"
			"\t-x  Output pngs with filename in hex\n"
			"\t-c  Config file name\n"
		);
}

void
parse_arguments (int argc, char *argv[], struct options *opts)
{
	char ch;

	memset (opts, 0, sizeof (struct options));

	while (-1 != (ch = getopt (argc, argv, "h?n:o:mp0:v:xc:")))
	{
		switch (ch)
		{
			case 'o':
				opts->outdir = optarg;
				break;
			case 'm':
				opts->usemax = 1;
				break;
			case 'n':
				opts->prefix = optarg;
				break;
			case 'p':
				opts->print = 1;
				break;
			case '0':
				opts->zeropos = atoi (optarg);
				break;
			case 'v':
				opts->verbose = atoi (optarg);
				break;
			case 'x':
				opts->ishex = 1;
				break;
			case 'c':
				opts->config = optarg;
				break;
			case '?':
			case 'h':
			default:
				usage ();
				exit (EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
	{
		usage ();
		exit (EXIT_FAILURE);
	}
	opts->infile = argv[0];
	if (opts->outdir == NULL)
		opts->outdir = ".";
}

index_header_t *
readIndex (uint8_t *buf)
{
	header_t *fh = (header_t *)buf;
	index_header_t *h;
	int i;

	fh->magic = get_32_le (&fh->magic);
	if (fh->magic != 0xffffffff)
	{
		verbose (1, "File is not a valid .fnt file.\n");
		exit (EXIT_FAILURE);
	}

	fh->magic_two = get_32_le (&fh->magic_two);
	if (fh->magic_two != 0x00000000)
	{
		verbose (1, "File is not a valid .fnt file.\n");
		exit (EXIT_FAILURE);
	}

	h = malloc (sizeof (index_header_t));
	if (!h)
	{
		verbose (1, "Out of memory parsing file header\n");
		exit (EXIT_FAILURE);
	}

	h->data_ofs = sizeof (*fh);
	h->num_chars = MAX_FONT_CHARS; // count of chars never changes
	h->first_char = 32; // first char is always ' ' (space)
	h->leading = fh->leading;
	h->max_ascend = fh->maxascender;
	h->max_descend = fh->maxdescender;
	h->spacing = fh->charspacing;
	h->kernamount = fh->kernamount;

	h->chars = malloc (h->num_chars * sizeof (char_info_t));
	if (!h->chars)
	{
		verbose (1, "Out of memory parsing char descriptors\n");
		exit (EXIT_FAILURE);
	}
	memset (h->chars, 0, h->num_chars * sizeof (char_info_t));

	for (i = 0; i < h->num_chars; i++)
	{
		char_info_t *info = h->chars + i;

		info->kerning = fh->kerntab[i];
	}

	for (i = 0; i < 2; i++)
	{
		uint8_t pad[2];
		pad[i] = fh->padding[i];
	}

	return h;
}

void
freeChar (char_info_t *f)
{
	if ((f->malloced & 1) && f->data)
		free (f->data);
}

void
freeIndex (index_header_t *h)
{
	int i;

	if (!h)
		return;
	if (h->chars)
	{
		for (i = 0; i < h->num_chars; ++i)
			freeChar (h->chars + i);

		free (h->chars);
	}
	free (h);
}

void
printIndex (const index_header_t *h, const uint8_t *buf, FILE *out)
{
	fprintf (out, "0x%08x  Leading: %d\n",
			offsetof (header_t, leading), h->leading);
	fprintf (out, "0x%08x  MaxAscend: %d\n",
			offsetof (header_t, maxascender), h->max_ascend);
	fprintf (out, "0x%08x  MaxDescend: %d\n",
			offsetof (header_t, maxdescender), h->max_descend);
	fprintf (out, "0x%08x  CharSpacing: %d\n",
			offsetof (header_t, charspacing), h->spacing);
	fprintf (out, "0x%08x  KernAmount: %d\n",
			offsetof (header_t, kernamount), h->kernamount);
	fprintf (out, "0x%08x  Number of chars: %d\n",
			offsetof (header_t, kerntab), MAX_FONT_CHARS);

	(void)buf;
}

int
readChar (char_info_t *f, const uint8_t *buf)
{
	uint32_t smask, smask_s, smask_n;
	uint8_t dmask;
	int x, y;
	const uint8_t *src;
	uint8_t *dst;
	int consumed = 0;
	int sawPixelsY;

	f->xsize = (f->w + 7) / 8;
	f->size = f->xsize * f->datah;
	f->data = malloc (f->size);
	if (!f->data)
	{
		verbose (1, "Out of memory reading char\n");
		return -1;
	}
	memset (f->data, 0, f->size);
	f->malloced |= 1;

	if (f->w <= 8)
	{	// minimum of 4 bits per line
		smask_s = 0x80;
		smask_n = 0x08;
	}
	else
	{	// 16 bits per line with first pixel skipped
		smask_s = 0x40;
		smask_n = 0x100; // never used
	}

	for (y = f->dy, smask = 0, src = buf - 1; y < f->dy + f->h; ++y)
	{
		dst = f->data + f->xsize * y;

		if (smask >= smask_n)
			smask = smask_n;
		else
		{
			smask = smask_s;
			++src;
			++consumed;
		}

		sawPixelsY = 0;
		for (x = 0, dmask = 0x80; x < f->w; ++x, smask >>= 1, dmask >>= 1)
		{
			if (!smask)
			{	// next mask byte
				smask = 0x80;
				++src;
				++consumed;
			}
			if (!dmask)
			{	// next mask byte
				dmask = 0x80;
				++dst;
			}

			if (*src & smask)
			{
				*dst |= dmask;
				sawPixelsY = 1;
			}
		}

		if (sawPixelsY)
			f->acth = y + 1;
	}

	if (f->acth < 1)
		f->acth = 1; // at least 1 pixel high

	return consumed;
}

void readChars (index_header_t *h, const uint8_t *buf)
{
	int i;
	int ret;

	buf += h->data_ofs;

	for (i = 0; i < h->num_chars; ++i)
	{
		char_info_t *info = h->chars + i;

		if (info->w <= 0)
			continue;

		ret = readChar (info, buf);
		if (ret <= 0)
		{
			verbose (1,
					"Char %d conversion failed. Buffer is most likely"
					"desynced and remainng chars broken\n", i);
			ret = 0;
		}
		buf += ret;
	}
}

void
calcMaxHeight (index_header_t *h)
{
	int i;
	int max_h;

	for (i = 0, max_h = 0; i < h->num_chars; ++i)
	{
		char_info_t *info = h->chars + i;

		if (info->w <= 0)
			continue;

		if (info->acth > max_h)
			max_h = info->acth;
	}

	h->max_height = max_h;
}

void
updateCharHeights (index_header_t *h, int bDataH)
{
	int i;

	for (i = 0; i < h->num_chars; ++i)
	{
		char_info_t *info = h->chars + i;

		if (info->w <= 0)
			continue;

		info->acth = bDataH ? info->datah : h->max_height;
	}
}

void
writeBitmapMask (const char *filename, const char_info_t *f)
{
	uint8_t **lines;
	int y;

	lines = alloca (f->datah * sizeof (uint8_t *));

	for (y = 0; y < f->acth; ++y)
		lines[y] = f->data + f->xsize * y;

	{
		FILE *file;
		png_structp png_ptr;
		png_infop info_ptr;
		png_color_16 trans;

		png_ptr = png_create_write_struct
				(PNG_LIBPNG_VER_STRING, (png_voidp)NULL /* user_error_ptr */,
				NULL /* user_error_fn */, NULL /* user_warning_fn */);
		if (!png_ptr)
		{
			verbose (1, "png_create_write_struct failed.\n");
			exit (EXIT_FAILURE);
		}

		info_ptr = png_create_info_struct (png_ptr);
		if (!info_ptr)
		{
			png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
			verbose (1, "png_create_info_struct failed.\n");
			exit (EXIT_FAILURE);
		}
		if (setjmp (png_jmpbuf (png_ptr)))
		{
			verbose (1, "png error.\n");
			png_destroy_write_struct (&png_ptr, &info_ptr);
			exit (EXIT_FAILURE);
		}

		file = fopen (filename, "wb");
		if (!file)
		{
			verbose (1, "Could not open file '%s': %s\n",
					filename, strerror (errno));
			png_destroy_write_struct (&png_ptr, &info_ptr);
			exit (EXIT_FAILURE);
		}
		png_init_io (png_ptr, file);
		png_set_IHDR (png_ptr, info_ptr, f->w, f->acth,
				1 /* bit_depth per channel */, PNG_COLOR_TYPE_GRAY,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
		trans.gray = 0;
		png_set_tRNS (png_ptr, info_ptr, NULL, 0, &trans);
		png_write_info (png_ptr, info_ptr);
		png_write_image (png_ptr, (png_byte **)lines);
		png_write_end (png_ptr, info_ptr);
		png_destroy_write_struct (&png_ptr, &info_ptr);
		fclose (file);
	}
}

void
writeFiles (const index_header_t *h, const char *path, const char *prefix,
			int zeropos, int ishex, const char *cfg, const char *infile)
{
	int i;
	char filename[512];
	struct stat sb;
	char fmt[32] = "%s/%s%";
	char configPath[512];
	FILE *config = NULL;

	if (!(stat (path, &sb) == 0 && S_ISDIR (sb.st_mode)))
	{
		printf ("Path not found, creating path.\n");
		MKDIR (path, 777);
	}

	if (cfg)
	{
		sprintf (configPath, "%s/%s", path, cfg);

		config = fopen (configPath, "wb");
		if (!config)
		{
			verbose (1, "Could not open file '%s': %s\n",
					 configPath, strerror (errno));
			fclose (config);
			exit (EXIT_FAILURE);
		}
	}

	if (zeropos > 0)
		sprintf (fmt + strlen (fmt), ishex ? "0%dx" : "0%dd", zeropos);
	else
		strcat (fmt, ishex ? "x" : "d");
	strcat (fmt, ".%s");

	for (i = 0; i < h->num_chars; i++)
	{
		char_info_t *info = h->chars + i;

		if (info->w <= 0)
			continue;

		sprintf (filename, fmt, path, prefix, h->first_char + i, "png");

		writeBitmapMask (filename, h->chars + i);

		if (config)
		{
			//int fullKern;
			int kernLBits = (info->kerning >> 2) & 3;
			int kernRBits = info->kerning & 3;

			// fullKern = (kernLBits << 2) | kernRBits;

			if (i == 0)
			{
				fprintf (config, "%s %d %d %d\n", infile,
						 h->height + h->leading, h->spacing, h->kerning);
			}
			fprintf (config, "%05x %d %d\n", h->first_char + i, kernLBits, kernRBits);
		}
	}

	if (config)
		fclose (config);
}

uint16_t
get_16_le (uint16_t *val)
{	// Unused
	return (uint16_t)(((uint8_t *)val)[1]) << 8 | ((uint8_t *)val)[0];
}

uint32_t
get_32_le (uint32_t *val)
{
	return (uint32_t)(((uint16_t *)val)[1]) << 16 | ((uint16_t *)val)[0];
}

uint8_t
get_quad (const uint8_t *table, int index)
{
	uint8_t val = table[index / 2];
	return (index % 2) ? (val & 0x0f) : (val >> 4);
}
