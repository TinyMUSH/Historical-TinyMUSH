
/*
 * Compression library. Uses a radix tree to compress substrings
 * down to 12 bit codes.
 */

#include <stdio.h>
#include "radix.h"

#include "compresstab.h"

static struct r_node *root;
static unsigned char *decompresstab[4096];
static unsigned char singletons[260];

unsigned int strings_compressed = 0;
unsigned int strings_decompressed = 0;
unsigned int chars_in = 0;
unsigned int symbols_out = 0;


void init_string_compress()
{
	int code, i;
	unsigned char *p;


	root = (struct r_node *)malloc(sizeof(struct r_node));

	root->count = 0;

	/*
	 * Get all the printables 
	 */

	p = singletons;
	*p = '\0';
	decompresstab[0] = p;
	p++;

	for (i = 1; i < 128; i++) {
		p[0] = (unsigned char)i;
		p[1] = '\0';
		code = r_insert(&root, p);
		if (code <= 0 || code > 4095) {
			printf("Invalid code 1\n");
			abort();
		}
		decompresstab[code] = p;
		p += 2;
	}

	/*
	 * Now insert all the strings in our compression table 
	 */

	for (i = 0; cmptab[i] != NULL; i++) {
		code = r_insert(&root, (unsigned char *)(cmptab[i]));
		if (code <= 0 || code > 4095) {
			printf("Invalid code 2\n");
			abort();
		}
		p = (unsigned char *)(cmptab[i]);
		decompresstab[code] = p;
	}
#ifdef DEBUG
	printf("Done inserting.\n");
#endif

}


/*
 * Compress the input string to the output array as 12 bit codes.
 * Return the number of bytes to worry about in the output.
 */

int string_compress(src, dst)
unsigned char *src;
unsigned char *dst;
{
	int code;
	int bitnum = 0;
	int bytenum;

#ifdef DEBUG
	int i;

#endif

	strings_compressed++;
	chars_in += strlen(src) + 1;

	do {
		code = r_compress(root, &src);
#ifdef DEBUG
		printf("emit code %x\n", code);
#endif
		bytenum = (bitnum >> 3);
		if (bitnum & 7) {
			dst[bytenum] = dst[bytenum] | ((code >> 8) & 0x0f);
			dst[bytenum + 1] = (code & 0xff);
		} else {
			dst[bytenum] = (code >> 4) & 0xff;
			dst[bytenum + 1] = ((code << 4) & 0xf0);
		}
		bitnum += 12;
		symbols_out++;
	} while (code != 0);

#ifdef DEBUG
	bytenum = ((bitnum & 7) ? (bitnum >> 3) + 1 : (bitnum >> 3));
	for (i = 0; i < bytenum; i++) {
		printf("%x ", dst[i]);
	}
	printf("\n");
#endif

	return ((bitnum & 7) ? (bitnum >> 3) + 1 : (bitnum >> 3));
}

/*
 * Decompress an array of 12 bit codes as produced by string_compress().
 * Return the number of bytes in the string, including zero terminator.
 */

int string_decompress(src, dst)
unsigned char *src;
unsigned char *dst;
{
	unsigned int code, bitnum, bytenum;
	register unsigned char *p, tmp;
	int count = 0;

	strings_decompressed++;

	bitnum = 0;
	while (1) {
		bytenum = bitnum >> 3;
		if (bitnum & 7) {
			code = (tmp & 0x0f) << 8;
			code |= src[bytenum + 1];
		} else {
			code = src[bytenum] << 4;
			tmp = src[bytenum + 1];
			code |= tmp >> 4;
		}
#ifdef DEBUG
		printf("Extract code %x = '%s'\n",
		       code, decompresstab[code]);
#endif
		if (code == 0)
			break;

		p = decompresstab[code];
		while (*p) {
			count++;
			*dst++ = *p++;
		}
		bitnum += 12;
	}
	*dst = '\0';
	return count + 1;	/*
				 * Include zero terminator 
				 */
}
