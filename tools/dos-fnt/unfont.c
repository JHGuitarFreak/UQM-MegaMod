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
#	define makedir(d)  _mkdir(d)
#	define inline __inline
#else
#	include <unistd.h>
#	define makedir(d)  mkdir(d, 0777)
#endif
#if defined(__GNUC__)
#	include <alloca.h>
#	define inline __inline__
#endif 
#include <png.h>

#define countof(a)	   ( sizeof(a)/sizeof(*a) )
#ifndef offsetof
#	define offsetof(s,m)  ( (size_t)(&((s*)0)->m) )
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
	uint32_t PACKED magic;  // Always ffff
   	uint8_t  PACKED height;
	uint8_t  PACKED baseline;
	uint8_t  PACKED kern_amount;  // char spacing and kerning - upper and lower nibble
	uint8_t  PACKED char_w[MAX_FONT_CHARS / 2];  // 2 chars per byte; upper and lower nibble
	uint8_t  PACKED kerntab[MAX_FONT_CHARS / 2]; // 2 chars per byte; upper and lower nibble
	uint8_t  PACKED char_dy[MAX_FONT_CHARS / 2]; // 2 chars per byte; upper and lower nibble
} header_t;

typedef struct
{
	size_t size;
	uint8_t* data;
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
	int baseline;
	int max_descend;
	int max_height;
	int spacing;
	int kerning;
	uint32_t data_ofs;
	int first_char;
	int num_chars;
	char_info_t* chars;
} index_header_t;

#if defined(_MSC_VER)
#	pragma pack(pop)
#endif

struct options
{
	char *infile;
	char *outdir;
	char *prefix;
	int makeani;
	int list;
	int print;
	int verbose;
	int usemax;
	int zeropos;
};

int verbose_level = 0;
void verbose(int level, const char* fmt, ...);

index_header_t* readIndex(uint8_t *buf);
void freeIndex(index_header_t *);
void parse_arguments(int argc, char *argv[], struct options *opts);
void printIndex(const index_header_t *, const uint8_t *buf, FILE *out);
void readChars(index_header_t *, const uint8_t *buf);
void calcMaxHeight(index_header_t *);
void updateCharHeights(index_header_t *, int bDataH);
void writeFiles(const index_header_t *, const char *path, const char *prefix, int zeropos);

inline uint16_t get_16_le(uint16_t* val);
inline uint32_t get_32_le(uint32_t* val);

inline uint8_t get_quad(const uint8_t *table, int index);

int main(int argc, char *argv[])
{
	FILE* in;
	size_t inlen;
	uint8_t *buf;
	index_header_t *h;
	struct options opts;
	char *prefix;

	parse_arguments(argc, argv, &opts);
	verbose_level = opts.verbose;

	in = fopen(opts.infile, "rb");
	if (!in)
	{
		verbose(1, "Error: Could not open file %s: %s\n",
				opts.infile, strerror(errno));
		return EXIT_FAILURE;
	}
	
	fseek(in, 0, SEEK_END);
	inlen = ftell(in);
	fseek(in, 0, SEEK_SET);

	buf = malloc(inlen);
	if (!buf)
	{
		verbose(1, "Out of memory reading file\n");
		return EXIT_FAILURE;
	}
	if (inlen != fread(buf, 1, inlen, in))
	{
		verbose(1, "Cannot read file '%s'\n", opts.infile);
		return EXIT_FAILURE;
	}
	fclose(in);

	h = readIndex(buf);
	if (opts.print)
		printIndex(h, buf, stdout);
	
	prefix = opts.prefix;
	if (!prefix)
		prefix = "";

	if (!opts.print && !opts.list && !opts.makeani)
	{
		size_t len;
				
		len = strlen(opts.outdir);
		if (opts.outdir[len - 1] == '/')
			opts.outdir[len - 1] = '\0';

		readChars(h, buf);
		calcMaxHeight(h);
		if (opts.usemax)
			updateCharHeights(h, 0);

		writeFiles(h, opts.outdir, prefix, opts.zeropos);
	}

	free(buf);
	
	return EXIT_SUCCESS;
}

void verbose(int level, const char* fmt, ...)
{
	va_list args;
	
	if (verbose_level < level)
		return;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void usage()
{
	fprintf(stderr,
			//"unfont -a [-m] [-0] <infile>\n"
			"unfont -p <infile>\n"
			"unfont [-m] [-0] [-o <outdir>] [-n <prefix>] <infile>\n"
			"Options:\n"
			"\t-p  print header and frame info\n"
			"\t-o  stuff pngs into <outdir>\n"
			"\t-n  name pngs <prefix><N>.png\n"
			"\t-m  char images will be of maximum seen height\n"
			"\t-0  make <N> 0-prepended; use several to specify width\n"
			"\t-v  increase verbosity level (use more than once)\n"
			);
}

void parse_arguments(int argc, char *argv[], struct options *opts)
{
	char ch;
	
	memset(opts, 0, sizeof (struct options));

	while (-1 != (ch = getopt(argc, argv, "ah?ln:o:mp0v")))
	{
		switch (ch)
		{
		case 'a':
			opts->makeani = 1;
			break;
		case 'l':
			//opts->list = 1;
			break;
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
			opts->zeropos++;
			break;
		case 'v':
			opts->verbose++;
			break;
		case '?':
		case 'h':
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
	{
		usage();
		exit(EXIT_FAILURE);
	}
	opts->infile = argv[0];
	if (opts->outdir == NULL)
		opts->outdir = ".";
}

index_header_t* readIndex(uint8_t *buf)
{
	header_t* fh = (header_t*) buf;
	const uint8_t *bufptr;
	index_header_t* h;
	int i;

	fh->magic = get_32_le(&fh->magic);
	if (fh->magic != 0xffffffff)
	{
		verbose(1, "File is not a valid dos .fnt file.\n");
		exit(EXIT_FAILURE);
	}

	h = malloc(sizeof(index_header_t));
	if (!h)
	{
		verbose(1, "Out of memory parsing file header\n");
		exit(EXIT_FAILURE);
	}

	h->data_ofs = sizeof(*fh);
	h->num_chars = MAX_FONT_CHARS; // count of chars never changes
	h->first_char = 32; // first char is always ' ' (space)
	h->height = fh->height;
	h->baseline = fh->height - fh->baseline;
	h->spacing = (fh->kern_amount & 0xf0) >> 4;
	h->kerning = (fh->kern_amount & 0x0f);
	
	bufptr = buf + sizeof(*fh);
	h->chars = malloc(h->num_chars * sizeof(char_info_t));
	if (!h->chars)
	{
		verbose(1, "Out of memory parsing char descriptors\n");
		exit(EXIT_FAILURE);
	}
	memset(h->chars, 0, h->num_chars * sizeof(char_info_t));

	h->max_descend = 0;
	for (i = 0; i < h->num_chars; ++i)
	{
		int dy = get_quad(fh->char_dy, i);
		if (dy > h->max_descend)
			h->max_descend = dy;
	}
	h->max_height = h->height + h->max_descend;

	for (i = 0; i < h->num_chars; ++i)
	{
		char_info_t* info = h->chars + i;
		
		info->h = h->height;
		info->datah = h->max_height;
		info->w = get_quad(fh->char_w, i);
		info->kerning = get_quad(fh->kerntab, i);
		info->dy = get_quad(fh->char_dy, i);
	}

	return h;
}

void freeChar(char_info_t* f)
{
	if ((f->malloced & 1) && f->data)
		free(f->data);
}

void freeIndex(index_header_t* h)
{
	int i;

	if (!h)
		return;
	if (h->chars)
	{
		for (i = 0; i < h->num_chars; ++i)
			freeChar(h->chars + i);
		
		free(h->chars);
	}
	free(h);
}

void printIndex(const index_header_t *h, const uint8_t *buf, FILE *out)
{
	fprintf(out, "0x%08x  Height: 0x%02x\n",
			offsetof(header_t, height), h->height);
	fprintf(out, "0x%08x  Baseline: 0x%02x\n",
			offsetof(header_t, baseline), h->baseline);
	fprintf(out, "              Max Descend: %d\n",
			h->max_descend);
	fprintf(out, "0x%08x  Spacing: 0x%02x\n",
			offsetof(header_t, kern_amount), h->spacing);
	fprintf(out, "0x%08x  Kerning: 0x%02x\n",
			offsetof(header_t, kern_amount), h->kerning);
	fprintf(out, "0x%08x  Number of chars:        %d\n",
			offsetof(header_t, char_w), MAX_FONT_CHARS);

	(void)buf;
}

int readChar(char_info_t *f, const uint8_t *buf)
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
	f->data = malloc(f->size);
	if (!f->data)
	{
		verbose(1, "Out of memory reading char\n");
		return -1;
	}
	memset(f->data, 0, f->size);
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

void readChars(index_header_t *h, const uint8_t *buf)
{
	int i;
	int ret;

	buf += h->data_ofs;

	for (i = 0; i < h->num_chars; ++i)
	{
		char_info_t* info = h->chars + i;

		if (info->w <= 0)
			continue;

		ret = readChar(info, buf);
		if (ret <= 0)
		{
			verbose(1, "Char %d conversion failed. Buffer is most likely desynced and remainng chars broken\n", i);
			ret = 0;
		}
		buf += ret;
	}
}

void calcMaxHeight(index_header_t *h)
{
	int i;
	int max_h;

	for (i = 0, max_h = 0; i < h->num_chars; ++i)
	{
		char_info_t* info = h->chars + i;

		if (info->w <= 0)
			continue;

		if (info->acth > max_h)
			max_h = info->acth;
	}

	h->max_height = max_h;
}

void updateCharHeights(index_header_t *h, int bDataH)
{
	int i;

	for (i = 0; i < h->num_chars; ++i)
	{
		char_info_t* info = h->chars + i;

		if (info->w <= 0)
			continue;

		info->acth = bDataH ? info->datah : h->max_height;
	}
}

#if 1
// this version writes out 1bit PNGs
void writeBitmapMask(const char *filename, const char_info_t* f)
{
	uint8_t **lines;
	int y;

	lines = alloca(f->datah * sizeof (uint8_t *));

	for (y = 0; y < f->acth; ++y)
		lines[y] = f->data + f->xsize * y;

	{
		FILE *file;
		png_structp png_ptr;
		png_infop info_ptr;
		png_color_16 trans;

		png_ptr = png_create_write_struct
				(PNG_LIBPNG_VER_STRING, (png_voidp) NULL /* user_error_ptr */,
				NULL /* user_error_fn */, NULL /* user_warning_fn */);
		if (!png_ptr)
		{
			verbose(1, "png_create_write_struct failed.\n");
			exit(EXIT_FAILURE);
		}
	
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
			verbose(1, "png_create_info_struct failed.\n");
			exit(EXIT_FAILURE);
		}
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			verbose(1, "png error.\n");
			png_destroy_write_struct(&png_ptr, &info_ptr);
			exit(EXIT_FAILURE);
		}

		file = fopen(filename, "wb");
		if (!file)
		{
			verbose(1, "Could not open file '%s': %s\n",
					filename, strerror(errno));
			png_destroy_write_struct(&png_ptr, &info_ptr);
			exit(EXIT_FAILURE);
		}
		png_init_io(png_ptr, file);
		png_set_IHDR(png_ptr, info_ptr, f->w, f->acth,
				1 /* bit_depth per channel */, PNG_COLOR_TYPE_GRAY,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
		trans.gray = 0;
		png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans);
		png_write_info(png_ptr, info_ptr);
		png_write_image(png_ptr, (png_byte **) lines);
		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(file);
	}
}
#else
// this version writes out 8bit PNGs with 2-color pal
void writeBitmapMask(const char *filename, const char_info_t* f)
{
	uint32_t bufsize;
	uint8_t *buf;
	const uint8_t *src1;
	uint8_t mask;
	uint8_t *dst;
	uint8_t **lines;
	int x, y;
	
	bufsize = f->w * f->h * sizeof(uint8_t);
	buf = alloca(bufsize);
	if (!buf)
	{
		verbose(1, "Out of stack while writing image\n");
		exit(EXIT_FAILURE);
	}
	memset(buf, 0, bufsize);

	lines = alloca(f->h * sizeof(uint8_t *));

	for (y = 0; y < f->h; ++y)
	{
		src1 = f->data + desc->xsize * y;
		dst = buf + f->w * y;
		lines[y] = dst;

		for (x = 0, mask = 0x80; x < f->w; ++x, ++dst, mask >>= 1)
		{
			if (!mask)
			{	// next mask byte
				mask = 0x80;
				++src1;
			}

			*dst = (*src1 & mask) ? 1 : 0;
		}
	}

	{
		FILE *file;
		png_structp png_ptr;
		png_infop info_ptr;
		png_color_8 sig_bit;
		png_color bmppal[2] = {{0x00, 0x00, 0x00}, {0xff, 0xff, 0xff}};
		png_byte trans[2] = {0x00, 0xff};
	
		png_ptr = png_create_write_struct
				(PNG_LIBPNG_VER_STRING, (png_voidp) NULL /* user_error_ptr */,
				NULL /* user_error_fn */, NULL /* user_warning_fn */);
		if (!png_ptr)
		{
			verbose(1, "png_create_write_struct failed.\n");
			exit(EXIT_FAILURE);
		}
	
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
			verbose(1, "png_create_info_struct failed.\n");
			exit(EXIT_FAILURE);
		}
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			verbose(1, "png error.\n");
			png_destroy_write_struct(&png_ptr, &info_ptr);
			exit(EXIT_FAILURE);
		}

		file = fopen(filename, "wb");
		if (!file)
		{
			verbose(1, "Could not open file '%s': %s\n",
					filename, strerror(errno));
			png_destroy_write_struct(&png_ptr, &info_ptr);
			exit(EXIT_FAILURE);
		}
		png_init_io(png_ptr, file);
		png_set_IHDR(png_ptr, info_ptr, f->w, f->h,
				8 /* bit_depth per channel */, PNG_COLOR_TYPE_PALETTE,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
		sig_bit.red = 8;
		sig_bit.green = 8;
		sig_bit.blue = 8;
		png_set_sBIT(png_ptr, info_ptr, &sig_bit);
		png_set_PLTE(png_ptr, info_ptr, bmppal, 2);
		// generate transparency chunk
		// only need to write out upto and including transparent index
		png_set_tRNS(png_ptr, info_ptr, trans, 1, NULL);
		png_write_info(png_ptr, info_ptr);
		png_write_image(png_ptr, lines);
		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(file);
	}
}
#endif // 0 or 1

void writeFiles(const index_header_t *h, const char *path, const char *prefix, int zeropos)
{
	int i;
	char filename[512];
	char fmt[32] = "%s/%s%";

	if (zeropos > 0)
		sprintf(fmt + strlen(fmt), "0%dd", zeropos);
	else
		strcat(fmt, "d");
	strcat(fmt, ".%s");
		

	for (i = 0; i < h->num_chars; i++)
	{
		char_info_t* info = h->chars + i;

		if (info->w <= 0)
			continue;

		sprintf(filename, fmt, path, prefix, h->first_char + i, "png");
		writeBitmapMask(filename, h->chars + i);
	}
}


inline uint16_t get_16_le(uint16_t* val)
{
	return (uint16_t)(((uint8_t*)val)[1]) << 8 | ((uint8_t*)val)[0];
}

inline uint32_t get_32_le(uint32_t* val)
{
	return (uint32_t)(((uint16_t*)val)[1]) << 16 | ((uint16_t*)val)[0];
}

inline uint8_t get_quad(const uint8_t *table, int index)
{
	uint8_t val = table[index / 2];
	return (index % 2) ? (val & 0x0f) : (val >> 4);
}
