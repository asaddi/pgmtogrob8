/* pgmtogrob8 - read a portable graymap and output an 8-shade GROB

   Copyright (C) 1995 by Allan Saddi (asaddi@pasteur.EECS.Berkeley.EDU)

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted, provided
   that the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  This software is provided "as is" without express or
   implied warranty. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm.h"

/* Our pretend version ID */
#define HP48_ROM_VERSION 'R'

/* HP binary object header */
#define PREPROLOG_LEN 8
static const unsigned char preprolog[PREPROLOG_LEN] =
{
  'H', 'P', 'H', 'P', '4', '8', '-', HP48_ROM_VERSION
};

/* Bit-reversed nibble table */
static const unsigned char revNibs[16] =
{
  0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
  0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};

#define GROB_DEPTH 3
#define GROB_COLOR (1 << GROB_DEPTH)

/* Encoded values for 8-shade GROBs using 4-2-1 weighting */
static const unsigned char grobPattern[GROB_COLOR] =
{
  0x07, 0x03, 0x05, 0x01, 0x06, 0x02, 0x04, 0x00
};

static void
saveGROB (FILE * grobFile, int width, int height,
	  unsigned char *bytePlane)
{
  int byteWidth;		/* Width of plane in bytes */
  unsigned char *lineBuf;	/* A GROB line */
  long bodySize;		/* Size of GROB body */
  unsigned char grobHeader[10];	/* GROB header buffer */
  int h, w;			/* Height and width counters */

  /* Calculate byteWidth */
  byteWidth = (width + 7) / 8;

  /* Allocate line buffer */
  if (!(lineBuf = calloc (byteWidth, 1)))
    pm_error ("out of memory allocating a row");

  /* Write out binary header */
  if (fwrite (preprolog, sizeof (preprolog[0]), PREPROLOG_LEN, grobFile) != PREPROLOG_LEN)
    pm_error ("write error");

  /* Setup GROB header */

  /* Calculate size of GROB minus prologs */
  bodySize = byteWidth * height * 2 + 15;

  grobHeader[0] = 0x1E;		/* Graphic object */
  grobHeader[1] = 0x2B;		/* Ditto */
  grobHeader[2] = (bodySize & 0x0000F) << 4;	/* Size of grob */
  grobHeader[3] = (bodySize & 0x00FF0) >> 4;	/* Ditto */
  grobHeader[4] = (bodySize & 0xFF000) >> 12;	/* Ditto */
  grobHeader[5] = (long) height & 0x000FF;	/* Height */
  grobHeader[6] = ((long) height & 0x0FF00) >> 8;	/* Ditto */
  grobHeader[7] = ((long) width & 0x0000F) << 4 |	/* Width */
    ((long) height & 0xF0000) >> 16;	/* Height */
  grobHeader[8] = ((long) width & 0x00FF0) >> 4;	/* Width */
  grobHeader[9] = ((long) width & 0xFF000) >> 12;	/* Ditto */

  /* Write out header */
  if (fwrite (grobHeader, sizeof (grobHeader[0]), 10, grobFile) != 10)
    pm_error ("write error");

  /* Write out the body now */

  /* Go line by line */
  for (h = 0; h < height; h++)
    {
      /* Copy line to line buffer */
      memcpy (lineBuf, bytePlane, byteWidth);

      /* Swap nibbles and bits of each byte */
      for (w = 0; w < byteWidth; w++)
	{
	  lineBuf[w] = revNibs[lineBuf[w] & 0x0F] << 4 |
	    revNibs[(lineBuf[w] & 0xF0) >> 4];
	}

      /* Write out line */
      if (fwrite (lineBuf, sizeof (*lineBuf), byteWidth, grobFile) != byteWidth)
	pm_error ("write error");

      /* Advance plane pointer */
      bytePlane += byteWidth;
    }

  /* Free up line buffer */
  free (lineBuf);
}

static void
writePixel (unsigned char *plane, int byteWidth, int x, int y,
	    int state)
{
  unsigned char d = plane[byteWidth * y + x / 8];
  if (state)
    d |= 0x80 >> (x % 8);
  else
    d &= ~(0x80 >> (x % 8));
  plane[byteWidth * y + x / 8] = d;
}

#define SCALE_GRAY(g) ((unsigned long) g * GROB_COLOR / maxvalp1)

int
main (int argc, char *argv[])
{
  FILE *inFile;			/* Input file */
  int cols, rows;		/* Width and height */
  gray maxval;			/* Number of shades */
  unsigned long maxvalp1;	/* Number of shades, including 0 */
  gray **grayImage;		/* The image */
  int byteWidth;		/* Width of GROB in bytes */
  unsigned char *grobPlane[GROB_DEPTH];		/* GROB image memory */
  gray *line;			/* A row */
  int h, w;			/* Counters */
  int pat;			/* GROB shade pattern */
  int d;			/* GROB depth counter */

  /* Usual PGM init stuff */
  pgm_init (&argc, argv);

  /* Check if we have too many arguments */
  if (argc > 2)
    pm_usage ("[pgmfile]");

  /* Set up inFile accordingly */
  if (argc == 2)
    inFile = pm_openr (argv[1]);
  else
    inFile = stdin;

  /* Read in PGM file */
  grayImage = pgm_readpgm (inFile, &cols, &rows, &maxval);

  pm_close (inFile);

  /* Calculate GROB byte width */
  byteWidth = (cols + 7) / 8;

  /* Allocate GROB image buffer */
  if (!(grobPlane[0] = calloc (byteWidth * rows * GROB_DEPTH, 1)))
    pm_error ("out of memory allocating an array");

  /* Set up other GROB plane pointers */
  for (d = 1; d < GROB_DEPTH; d++)
    grobPlane[d] = grobPlane[d - 1] + byteWidth * rows;

  /* Increment maxval for proper scaling */
  maxvalp1 = (unsigned long) maxval + 1;

  /* Loop through each row */
  for (h = 0; h < rows; h++)
    {
      /* Set up current line */
      line = grayImage[h];
      for (w = 0; w < cols; w++)
	{
	  pat = grobPattern[SCALE_GRAY (*(line++))];

	  /* Write GROB planes accordingly */
	  for (d = 0; d < GROB_DEPTH; d++)
	    writePixel (grobPlane[d], byteWidth, w, h, pat & (1 << d));
	}
    }

  /* Now save the GROB */
  saveGROB (stdout, cols, rows * GROB_DEPTH, grobPlane[0]);

  exit (0);
}

/* VIEW8 - HP48G/S(?) program to view 8-shade GROBs

   Copyright (C) 1995 by Allan Saddi (asaddi@pasteur.EECS.Berkeley.EDU)

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted, provided
   that the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  This software is provided "as is" without express or
   implied warranty.

   Note: This program is ASC encoded.

%%HP: T(0)A(R)F(.);
"D9D202BA812BF8176040D9D20E97158813053040CBD30F30407FE30B9F063004
02CE3059230D00407FE30322307CC308DA16B4F06D9D20CA1302CE3013593CA0
31FC2E4CCD20243008F1466081AF018F1466081AF048F1466081AF028F146608
1AF038FB9760808F84F1433441000CA81AF00808601180820C323206E0080820
8322201B00100158081AF12EA1B521001503D1D31F0210081AF1014174D081AF
1081AF19CA1417FB081AF1014172B081AF1081AF19CACA1417B9081AF101417E
8081AF1081AF19CA141797081AF101417C60320108018FA51108087460647F8F
13C10132141808601180820C32EFF6E00808208320001B0010015801B5210015
43808085F8D341501B8210014A808758F14A808658F320808018FA511080861D
18A981CD81AF1081AF1AEA81AF00320408018FA511080861F28082404000C081
AF1C8BE81E581AF1081AF1ACA81AF00320408018FA511080862548AB04CF1B00
1001520308902F2A0C150030B906C181AF123422000EA1B521001503663030F1
5C081AF123432000EA1B52100150381AF10818F8181AF00320408018FA511080
860003438000CB81AF138BA00E71B00100152030F902D2B04150030C9060081A
F123432000EA1B5210015030130815C081AF123422000EA1B52100150381AF10
818F0181AF000144193B213044230B2130B2130D9EA"

VIEW8 Keys:
    Left/Right Arrow - Scroll image left/right if image width > 131 pixels
    Up/Down Arrow - Scroll image up/down if image height > 64 pixels
    Enter - Quit
*/
