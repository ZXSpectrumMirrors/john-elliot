/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "dskbits.h"
#include "plus3fs.h"

#ifdef __PACIFIC__
#define AV0 "MKP3FS"
#else
#define AV0 argv[0]
#endif

typedef unsigned char byte;

const char *dsktype = "dsk";
const char *compress = NULL;
const char *label = NULL;
int timestamp = 1;
int format = 180;
char fcbname[12];
PLUS3FS gl_fs;
int gl_uid = 0;
int cpmonly = 0;
int dosonly = 0;
int usr0 = 1;
int pause = 0;
unsigned int loader = 0;

static unsigned char boot_180[] =
{
        0x00, /* Disc type */
        0x00, /* Disc geometry */
        0x28, /* Tracks */
        0x09, /* Sectors */
        0x02, /* Sector size */
        0x01, /* Reserved tracks */
        0x03, /* ?Sectors per block */
        0x02, /* ?Directory blocks */
        0x2A, /* Gap length (R/W) */
        0x52  /* Gap length (format) */
};

static unsigned char boot_720[] =
{
        0x03, /* Disc type */
        0x81, /* Disc geometry */
        0x50, /* Tracks */
        0x09, /* Sectors */
        0x02, /* Sector size */
        0x02, /* Reserved tracks */
        0x04, /* ?Sectors per block */
        0x04, /* ?Directory blocks */
        0x2A, /* Gap length (R/W) */
        0x52  /* Gap length (format) */
};


static void report(const char *s)
{
        printf("%s\r", s);
        fflush(stdout);
}

static void report_end(void) 
{
        printf("\r%-79.79s\r", "");
        fflush(stdout);
}

void diewith(dsk_err_t err)
{
	fprintf(stderr, "Cannot write %-8.8s.%-3.3s: %s\n",
			fcbname, fcbname + 8, p3fs_strerror(err));

	p3fs_umount(gl_fs);
	exit(1);
}


dsk_err_t put_file(const char *source, const char *dest)
{
	FILE *fp;
	dsk_err_t err;
	PLUS3FILE fpo;
	int c;

	strncpy(fcbname, dest, 11);
	fcbname[11] = 0;
	fp = fopen(source, "rb");
	if (!fp) return DSK_ERR_SYSERR;

	printf("Writing %-8.8s.%-3.3s\n", fcbname, fcbname+8);

	err = p3fs_creat(gl_fs, &fpo, gl_uid, fcbname);
	if (err) return err;

	while ((c = fgetc(fp)) != EOF)
	{
		err = p3fs_putc(c, fpo);
		if (err) return err;
	}
	fclose(fp);
	return p3fs_close(fpo);
}


dsk_err_t put_blob(const byte *data, int len, const char *dest)
{
	dsk_err_t err;
	PLUS3FILE fpo;
	int n;

	strncpy(fcbname, dest, 11);
	fcbname[11] = 0;

	printf("Writing %-8.8s.%-3.3s\n", fcbname, fcbname+8);

	err = p3fs_creat(gl_fs, &fpo, gl_uid, fcbname);
	if (err) return err;

	for (n = 0; n < len; n++)
	{
		err = p3fs_putc(data[n], fpo);
		if (err) return err;
	}
	return p3fs_close(fpo);
}



void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s {options} snafile dskfile {romfile}\n\n"
			"Options are:\n"
			"  -usr0:      Run snapshot in USR0 mode (default)\n"
			"  -48basic:   Run snapshot in 48 BASIC mode\n"
			"  -base addr: Address of 14-byte final loader\n"
			"  -pause:     Pause after loading (custom ROM only)\n"
			"  -noscreen:  Leave screen alone while loading (custom ROM only)\n"
			"  -180:       Write a 180k image (default)\n"
			"  -720:       Write a 720k image\n"
			"  -cpmonly:   Write a disc only usable by +3DOS\n"
			"  -dosonly:   Write a disc only usable by PCDOS\n"
			"  -type type: Output file type (default=dsk)\n"
			"  -compress cmp: Output compression (default=none)\n"
			"  -label lbl: Set disk label (default=none)\n"
			"  -nostamps:  Omit file date stamps\n", av0);
      	 
	exit(1);
}


void parse_loader(const char *s, const char *av0)
{
	if (!isdigit(s[0])) syntax(av0);

	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	{
		if (!sscanf(s + 2, "%x", &loader)) syntax(av0);
	}
	else if (!sscanf(s, "%u", &loader)) syntax(av0);

}


/* BASIC loader */
static byte disk[] = 
{
	'P', 'L', 'U', 'S', '3', 'D', 'O', 'S',		/* 0x00 */
	0x1A, 0x01, 0x00, 0xB5, 0x01, 0x00, 0x00, 0x00,
	0x35, 0x01, 0x00, 0x00, 0x35, 0x01, 0x00, 0x00,	/* 0x10 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x20 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x30 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x40 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x50 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x60 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x70 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A,

	0x00, 0x0A, 0x28, 0x00,	/* Line 10 */
	0xE7, 0x37, 0x0E, 0x00, 0x00, 0x07, 0x00, 0x00, 0x3A,	/* BORDER 7: */
	0xDA, 0x37, 0x0E, 0x00, 0x00, 0x07, 0x00, 0x00, 0x3A,	/* PAPER 7: */
	0xD9, 0x30, 0x0E, 0x00, 0x00, 0x07, 0x00, 0x00, 0x3A,	/* INK 0: */
	0xFD, '2',  '5',  '3',  '4',  '3',  0x0E, 0x00, 0x00,	/* CLEAR 25343*/
	      0xFF, 0x62, 0x00, 0x0D,				
	0x00, 0x14, 0x2D, 0x00,	/* Line 20 */
	0xF4, '2',  '3',  '6',  '7',  '5',  0x0E, 0x00, 0x00,	/* POKE 23675,*/
	      0x7B, 0x5C, 0x00, 0x2C, '1',  '2',  '4',  0x0E,	/*  124: */
	      0x00, 0x00, 0x7C, 0x00, 0x00, 0x3A,		
	0xF4, '2',  '3',  '6',  '7',  '6',  0x0E, 0x00, 0x00,	/* POKE 23675,*/
	      0x7C, 0x5C, 0x00, 0x2C, '9',  '1',  0x0E,		/*  91: */
	      0x00, 0x00, 0x5B, 0x00, 0x00, 0x0D,		
	0x00, 0x1E, 0x8D, 0x00, /* Line 30 */
	0xF5, 0xAC, '2',  '1',  0x0E, 0x00, 0x00, 0x15, 0x00,   /* PRINT AT */
	      0x00, 0x2C, '0',  0x0E, 0x00, 0x00, 0x00, 0x00,   /*  21, 0; */
	      0x00, 0x3B, 0xDC, '1',  0x0E, 0x00, 0x00, 0x01,   /* BRIGHT 1; */
              0x00, 0x00, 0x3B, 0xDA, '0',  0x0E, 0x00, 0x00,	/* PAPER 0; */
	      0x00, 0x00, 0x00, 0x3B, 0xD9, '7',  0x0E, 0x00,   /* INK 7; */
	      0x00, 0x07, 0x00, 0x00, 0x3B, 0x22, 'S',  'n',
	      'a', 'p', 's', 'h', 'o', 't', ' ', 'l', 'o', 	/* "Snapshot */
	      'a', 'd', 'e', 'r', ' ', ' ', ' ', ' ', ' ', 	/* loader "; */
	      ' ', ' ', ' ', ' ', ' ', ' ', 0x22, 0x3B,
	      0xD9,  '2', 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00,	/* INK 2; */
	      0x3B, 0x22, 0x90, 0x22, 0x3B,			/* "/"; */
	      0xDA,  '6', 0x0E, 0x00, 0x00, 0x06, 0x00, 0x00,	/* PAPER 6; */
	      0x3B, 0x22, 0x91, 0x22, 0x3B,			/* "/"; */
	      0xD9,  '4', 0x0E, 0x00, 0x00, 0x04, 0x00, 0x00,	/* INK 4; */
	      0x3B, 0x22, 0x90, 0x22, 0x3B,			/* "/"; */
	      0xDA,  '5', 0x0E, 0x00, 0x00, 0x05, 0x00, 0x00,	/* PAPER 5; */
	      0x3B, 0x22, 0x91, 0x22, 0x3B,			/* "/"; */
	      0xD9,  '0', 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,	/* INK 0; */
	      0x3B, 0x22, 0x90, 0x8F, 0x22, 0x0D,		/* "/_" */
	0x00, 0x28, 0x13, 0x00,	/* Line 40 */
	0xF5, 0xAC, 0x30, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,	/* PRINT AT */
	      0x2C, 0x30, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0,0; */
	      0x3B, 0x0D,
	0x00, 0x32, 0x1A, 0x00,	/* Line 50 */
	0xEF, 0x22, 's', 'n', 'a', 'p', 'l', 'd', '.',		/* LOAD "sna */
	      'z', 'x', 'b', 0x22, 0xAF, '2', '5', '3', 	/* pld.zxb" */
	      '4', '4', 0x0E, 0x00, 0x00, 0x00, 0x63, 0x00,     /* CODE 25344 */
	      0x0D,
	0x00, 0x3C, 0x0E, 0x00,	/* Line 60 */
	0xF5, 0xC0, '2', '5', '3', '4', '4', 		/* PRINT USR 25344 */
	      0x0E, 0x00, 0x00, 0x00, 0x63, 0x00, 0x0D
};

static byte rsdisk[] = 
{
	'P', 'L', 'U', 'S', '3', 'D', 'O', 'S',		/* 0x00 */
	0x1A, 0x01, 0x00, 0xB5, 0x01, 0x00, 0x00, 0x00,
	0x35, 0x01, 0x00, 0x00, 0x35, 0x01, 0x00, 0x00,	/* 0x10 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x20 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x30 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x40 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x50 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x60 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x70 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A,

	0x00, 0x0A, 0x28, 0x00,	/* Line 10 */
	0xE7, 0x37, 0x0E, 0x00, 0x00, 0x07, 0x00, 0x00, 0x3A,	/* BORDER 7: */
	0xDA, 0x37, 0x0E, 0x00, 0x00, 0x07, 0x00, 0x00, 0x3A,	/* PAPER 7: */
	0xD9, 0x30, 0x0E, 0x00, 0x00, 0x07, 0x00, 0x00, 0x3A,	/* INK 0: */
	0xFD, '3',  '2',  '7',  '6',  '7',  0x0E, 0x00, 0x00,	/* CLEAR 32767*/
	      0xFF, 0x7F, 0x00, 0x0D,				
	0x00, 0x14, 0x2D, 0x00,	/* Line 20 */
	0xF4, '2',  '3',  '6',  '7',  '5',  0x0E, 0x00, 0x00,	/* POKE 23675,*/
	      0x7B, 0x5C, 0x00, 0x2C, '1',  '2',  '4',  0x0E,	/*  124: */
	      0x00, 0x00, 0x7C, 0x00, 0x00, 0x3A,		
	0xF4, '2',  '3',  '6',  '7',  '6',  0x0E, 0x00, 0x00,	/* POKE 23675,*/
	      0x7C, 0x5C, 0x00, 0x2C, '9',  '1',  0x0E,		/*  91: */
	      0x00, 0x00, 0x5B, 0x00, 0x00, 0x0D,		
	0x00, 0x1E, 0x8D, 0x00, /* Line 30 */
	0xF5, 0xAC, '2',  '1',  0x0E, 0x00, 0x00, 0x15, 0x00,   /* PRINT AT */
	      0x00, 0x2C, '0',  0x0E, 0x00, 0x00, 0x00, 0x00,   /*  21, 0; */
	      0x00, 0x3B, 0xDC, '1',  0x0E, 0x00, 0x00, 0x01,   /* BRIGHT 1; */
              0x00, 0x00, 0x3B, 0xDA, '0',  0x0E, 0x00, 0x00,	/* PAPER 0; */
	      0x00, 0x00, 0x00, 0x3B, 0xD9, '7',  0x0E, 0x00,   /* INK 7; */
	      0x00, 0x07, 0x00, 0x00, 0x3B, 0x22, 'S',  'n',
	      'a', 'p', 's', 'h', 'o', 't', ' ', 'l', 'o', 	/* "Snapshot */
	      'a', 'd', 'e', 'r', ' ', ' ', ' ', ' ', ' ', 	/* loader "; */
	      ' ', ' ', ' ', ' ', ' ', ' ', 0x22, 0x3B,
	      0xD9,  '2', 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00,	/* INK 2; */
	      0x3B, 0x22, 0x90, 0x22, 0x3B,			/* "/"; */
	      0xDA,  '6', 0x0E, 0x00, 0x00, 0x06, 0x00, 0x00,	/* PAPER 6; */
	      0x3B, 0x22, 0x91, 0x22, 0x3B,			/* "/"; */
	      0xD9,  '4', 0x0E, 0x00, 0x00, 0x04, 0x00, 0x00,	/* INK 4; */
	      0x3B, 0x22, 0x90, 0x22, 0x3B,			/* "/"; */
	      0xDA,  '5', 0x0E, 0x00, 0x00, 0x05, 0x00, 0x00,	/* PAPER 5; */
	      0x3B, 0x22, 0x91, 0x22, 0x3B,			/* "/"; */
	      0xD9,  '0', 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,	/* INK 0; */
	      0x3B, 0x22, 0x90, 0x8F, 0x22, 0x0D,		/* "/_" */
	0x00, 0x28, 0x13, 0x00,	/* Line 40 */
	0xF5, 0xAC, 0x30, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,	/* PRINT AT */
	      0x2C, 0x30, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0,0; */
	      0x3B, 0x0D,
	0x00, 0x32, 0x1A, 0x00,	/* Line 50 */
	0xEF, 0x22, 'r', 'o', 'm', 's', 'n', 'a', '.',		/* LOAD "rom */
	      'z', 'x', 'b', 0x22, 0xAF, '3', '2', '7', 	/* sna.zxb" */
	      '6', '8', 0x0E, 0x00, 0x00, 0x00, 0x80, 0x00,     /* CODE 32768 */
	      0x0D,
	0x00, 0x3C, 0x0E, 0x00,	/* Line 60 */
	0xF5, 0xC0, '3', '2', '7', '6', '8', 		/* PRINT USR 25344 */
	      0x0E, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0D
};


/* Normal snapshot loader */
static byte snapld[] = 
{
	'P', 'L', 'U', 'S', '3', 'D', 'O', 'S',		/* 0x00 */
	0x1A, 0x01, 0x00, 0x3F, 0x02, 0x00, 0x00, 0x03,
	0xBF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x10 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x20 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x30 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x40 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x50 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x60 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x70 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C,


	0x18, 0x14, 	/* JR main */
	0x10,		/* 0x82: 0x10 for USR0 mode, 0x30 for 48BASIC mode */
	0x00, 0x00,	/* 0x83: Address for 14-byte final loader, 
			 *       0 => Use SNA stack or screen as appropriate */
	'0', '0', 'A', ':', 
	'S', 'N', 'A', 'P', 'S', 'H', 'O', 'T', '.', 'S', 'N', 'A', 0xFF,
	
	0xC5,			/* PUSH BC */
	0x21, 0x00, 0x58,	/* LD HL, 5800h    ;Clear attributes */
	0x7E,			/* LD A, (HL)      ;to the same ink and */
	0xE6, 0x38,		/* AND 38h         ;paper colours. */
	0x0F, 0x0F, 0x0F,	/* RRCA x3 */
	0x5F,			/* LD E, A */
	0x7E,			/* LD A, (HL) */
	0xE6, 0xF8,		/* AND 0F8h */
	0xB3,			/* OR E */
	0x77, 			/* LD (HL), A */
	0x11, 0x01, 0x58,	/* LD DE, 5801h */
	0x01, 0x7F, 0x02,	/* LD BC, 027Fh */
	0xED, 0xB0,		/* LDIR */
	0xE1, 			/* POP HL          ; Get our load address */
	0x11, 0x00, 0x40,	/* LD DE, 4000h */
	0x01, 0xBF, 0x01,	/* LD BC, 01BFh */
	0xED, 0xB0,		/* LDIR		   ; Copy loader to video RAM */
	0xC3, 0x3A, 0x40,	/* JP MAIN2 */
/* .phase 403Ah */
	0xED, 0x73, 0xFC, 0x40, /* LD (40FCh), SP */
	0xCD, 0xD8, 0x40,	/* CALL 40D8h	   ; Select +3DOS mem config */
	0x01, 0x05, 0x00,	/* LD BC, 5	   ; File 0, shared-read mode */
	0x11, 0x02, 0x00,	/* LD DE, 2        ; Open file */
	0x21, 0x05, 0x40,	/* LD HL, filename */
	0xCD, 0x06, 0x01,	/* CALL DOS_OPEN */
	0xD2, 0xC2, 0x40,	/* JP NC, 40C2h	   ; Failed to open */
/* Load the registers */
	0x01, 0x00, 0x00,	/* LD BC, 0	   ; Load into bank 0 */
	0x11, 0x1B, 0x00,	/* LD DE, 27       ; 27 bytes */
	0x21, 0xFE, 0x40,	/* LD HL, 40FEh */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xC2, 0x40,	/* JP NC, 40C2h	   ; Failed to read */
/* Load the screen */
	0x01, 0x07, 0x00,	/* LD BC, 7	   ; Load into bank 7 */
	0x11, 0x00, 0x1B,	/* LD DE, 1B00h    ; 6912 bytes */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xC2, 0x40,	/* JP NC, 40C2h	   ; Failed to read */
/* Display the screen */
	0x01, 0xFD, 0x7F,	/* LD BC, 7FFDh */
	0xF3,			/* DI */
	0x3A, 0x5C, 0x5B,	/* LD A, (BANKM) */
	0xF6, 0x08,		/* OR 8		   ; Display shadow screen */
	0x32, 0x5C, 0x5B,	/* LD (BANKM), A */
	0xED, 0x79,		/* OUT (C), A */
	0xFB,			/* EI */	
	0x3A, 0x18, 0x41,	/* LD A, (regs+26) ; Border */
	0xD3, 0xFE,		/* Set border colour */
/* Load the 2k currently in use by +3DOS and BASIC */
	0x01, 0x00, 0x00,	/* LD BC, 0	   ; Load into bank 0 */
	0x11, 0x00, 0x08,	/* LD DE, 0800h    ; 2048 bytes */
	0x21, 0x00, 0x50,	/* LD HL, 05000h */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xC2, 0x40,	/* JP NC, 40C2h	   ; Failed to read */
/* Load the remainder of the SNA */
	0x01, 0x00, 0x00,	/* LD BC, 0	   ; Load into bank 0 */
	0x11, 0x00, 0x9D,	/* LD DE, 9D00h    ; Rest of the data */
	0x21, 0x00, 0x63,	/* LD HL, 06300h */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xC2, 0x40,	/* JP NC, 40C2h	   ; Failed to read */
/* Close the file */
	0x06, 0x00,		/* LD B, 0 */
	0xCD, 0x09, 0x01,	/* CALL DOS_CLOSE */
	0xCD, 0x9C, 0x01,	/* CALL DD_L_OFF_MOTOR */
/* And execute the snapshot */
	0xF3, 			/* DI */
	0x21, 0x00, 0x50,	/* LD HL, 5000h */
	0x11, 0x00, 0x5B,	/* LD DE, 5B00h */
	0x01, 0x00, 0x08,	/* LD BC, 800h */
	0xED, 0xB0,		/* LDIR */
	0x21, 0xFE, 0x40,	/* LD HL, regs */
	0x11, 0x00, 0xDB,	/* LD DE, 0DB00h  ; Last stage at 7:DB00 */
	0x01, 0xC1, 0x00,	/* LD BC, 0C1h */
	0xED, 0xB0,		/* LDIR */
	0xC3, 0x29, 0xDB,	/* JP 0DB29h */
/* DOS error handler */
	0xF5,			/* PUSH AF */
	0xCD, 0xEA, 0x40,	/* CALL 40EAh	; Back to BASIC environment */
	0xF1,			/* POP AF */
	0xFE, 0x14,		/* CP 20 */
	0x30, 0x02,		/* JR NC, +2 */
	0xC6, 0x25,		/* ADD A, 25h */
	0xC6, 0x18,		/* ADD A, 18h */
	0x32, 0xD7, 0x40,	/* LD (40D7h), A */
	0xED, 0x7B, 0xFC, 0x40,	/* LD SP, (40FCh) */
	0xCF, 0x00,		/* RST 8 */
/* Bank7: */
	0x01, 0xFD, 0x7F,	/* LD BC, 7FFDh */
	0xF3,			/* DI */
	0x3A, 0x5C, 0x5B,	/* LD A, (BANKM) */
	0xCB, 0xA7,		/* RES 4, A */
	0xF6, 0x07,		/* OR 7 */
	0x32, 0x5C, 0x5B,	/* LD (BANKM), A */
	0xED, 0x79,		/* OUT (C), A */
	0xFB,			/* EI */
	0xC9,			/* RET */
/* Bank0: */
	0x01, 0xFD, 0x7F,	/* LD BC, 7FFDh */
	0xF3,			/* DI */
	0x3A, 0x5C, 0x5B,	/* LD A, (BANKM) */
	0xCB, 0xE7,		/* SET 4, A */
	0xE6, 0xF8,		/* AND 0F8h */
	0x32, 0x5C, 0x5B,	/* LD (BANKM), A */
	0xED, 0x79,		/* OUT (C), A */
	0xFB,			/* EI */
	0xC9,			/* RET */

	0x00, 0x00,		/* Stack pointer */
/* regs: */
/*	.phase 0DB00h */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff,	/* 27 bytes for SNA header */
	
	0x00, 0x00,		/* AF */
	0xED, 0x79, 		/* OUT (C), A */
	0x01, 0x00, 0x00,	/* LD BC, 0 */
	0xF1, 			/* POP AF */
	0x31, 0x00, 0x00,	/* LD SP, 0 */
	0x00,			/* NOP ; becomes DI or EI */
	0xED, 0x45,		/* RETN */
/* Launch loaded snapshot */
	0x2A, 0x03, 0x40,	/* Loader address */
	0x7C,			/* LD A, H */
	0xB5,			/* OR L */
	0x20, 0x11,		/* JR NZ, sec1 */
	0x21, 0x00, 0x40,	/* LD HL, 4000h */
	0x3A, 0x18, 0xDB,	/* LD A, (0DB18h) ; SP high */
	0xFE, 0xC0,		/* CP 0C0h */
	0x30, 0x07,		/* JR NC, sec1    ; Default to screen */
	0x2A, 0x17, 0xDB,	/* LD HL, (0DB17h); SP */
	0x11, 0xF0, 0xFF,	/* LD DE, -16 */
	0x19,			/* ADD HL, DE */
	0x22, 0x9A, 0xDB,	/* LD (0DB9Ah), HL ;->final stage */
	0x22, 0x81, 0xDB,	/* LD (0DB81h), HL) */
	0x23, 0x23,		/* INC HL x2 */
	0x22, 0xBF, 0xDB,	/* LD (0DBBFh), HL ;->final stage code */
	0x3A, 0x02, 0x40,	/* LD A, (4002h)   ;Memory bank to use */
	0x32, 0xBD, 0xDB,	/* LD (0DBBDh), A */
	0x3A, 0x13, 0xDB,	/* LD A, (0DB13h)  ; IFF */
	0xE6, 0x04,		/* AND 4 */
	0x3E, 0xF3,		/* LD A, 0F3h	   ; DI */
	0x28, 0x02,		/* JR Z, +2 */
	0x3E, 0xFB,		/* LD A, 0FBh	   ; EI */
	0x32, 0x26, 0xDB,	/* LD (0DB26h), A */
	0x2A, 0x0D, 0xDB,	/* LD HL, (0DB0Dh); BC */
	0x22, 0x20, 0xDB,	/* LD (0DB20h), HL */
	0x2A, 0x17, 0xDB,	/* LD HL, (0DB17h); SP */
	0x22, 0x24, 0xDB,	/* LD (0DB24h), HL */
	0x2A, 0x15, 0xDB,	/* LD HL, (0DB15h); AF */
	0x22, 0x1B, 0xDB,	/* LD (0DB1Bh), HL */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h ; Shadow screen */		
	0x11, 0x00, 0x40,	/* LD DE, 04000h ; Main screen */		
	0x01, 0x00, 0x1B,	/* LD BC, 01B00h ; Screen size */		
	0xED, 0xB0,		/* LDIR */
	0x21, 0x1B, 0xDB,	/* LD HL, 0DB1Bh ; Final launcher */	
	0x11, 0x00, 0x00,	/* LD DE, 0	 ; Address calculated above */
	0x01, 0x0E, 0x00,	/* LD BC, 0Eh */
	0xED, 0xB0,		/* LDIR */
	0x31, 0x01, 0xDB,	/* LD SP, regs + 1 */
	0xE1, 0xD1, 0xC1, 0xF1,	/* POP HL, DE, BC, AF */
	0x08,			/* EX AF, AF' */
	0xD9,			/* EXX */
	0xE1, 0xD1, 0xC1,	/* POP HL, DE, BC */
	0xFD, 0xE1, 0xDD, 0xE1,	/* POP IY, IX */
	0xC1,			/* POP BC */
	0x31, 0x00, 0x00,	/* LD SP, 0	; Address calculated above */
	0x3A, 0x19, 0xDB,	/* LD A, (0DB19h) */
	0xB7, 			/* OR A */
	0x20, 0x04,		/* JR NZ, +4 */
	0xED, 0x46,		/* IM 0 */
	0x18, 0x09,		/* JR +9 */
	0x3D,			/* DEC A */
	0x20, 0x04,		/* JR NZ, +4 */
	0xED, 0x56,		/* IM 1 */
	0x18, 0x02,		/* JR +2 */
	0xED, 0x5E,		/* IM 2 */
	0x3A, 0x00, 0xDB,	/* LD A, (0DB00h) */
	0xED, 0x47,		/* LD I, A */
	0x3A, 0x14, 0xDB,	/* LD A, (0DB14h) */
	0xED, 0x4F,		/* LD R, A */
	0x01, 0xFD, 0x7F,	/* LD BC, 7FFDh */
	0x3E, 0x00,		/* LD A, 0 ; Set above */
	0xC3, 0x00, 0x00,	/* JP 0	   ; Set above */
};


/* Snapshot loader that also loads a custom ROM */
static byte romsna[] = 
{
	'P', 'L', 'U', 'S', '3', 'D', 'O', 'S',		/* 0x00 */
	0x1A, 0x01, 0x00, 0x4C, 0x02, 0x00, 0x00, 0x03,
	0xCC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x10 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x20 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x30 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x40 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x50 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x60 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* 0x70 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96,


	0x18, 0x25, 	/* JR main */
	0x00, 0x00,	/* 0x82: Address for 12-byte final loader, 
			 *       0 => Use SNA stack or screen as appropriate */
	'0', '0', 'A', ':', 
	'S', 'N', 'A', 'P', 'S', 'H', 'O', 'T', '.', 'S', 'N', 'A', 0xFF,
	'0', '0', 'A', ':', 
	'C', 'U', 'S', 'T', 'O', 'M', '.', 'R', 'O', 'M', 0xFF, 0xFF, 0xFF,
	0x00, 			/* 0xA6: Options */
	0xED, 0x73, 0x24, 0x81,	/* LD (8124h), SP */
	0xCD, 0x00, 0x81,	/* CALL 8100h	  ; RAM 7 */
	0x11, 0x1C, 0x00,	/* LD DE, 1Ch	  ; +3DOS cache */
	0x21, 0x04, 0x1C,	/* LD HL, 1C04h   ; and RAMDISK */
	0xCD, 0x3F, 0x01,	/* CALL DOS SET 1346 */
/* Load the ROM image into bank 4 */
	0x01, 0x05, 0x00,	/* LD BC, 5      ; Open file */
	0x11, 0x01, 0x00,	/* LD DE, 1      ; in shared-read mode */
	0x21, 0x15, 0x80,	/* LD HL, fname  ; Filename */
	0xCD, 0x06, 0x01,	/* CALL DOS_OPEN */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x01, 0x04, 0x00,	/* LD BC, 4      ; Load ROM into bank 4 */
	0x11, 0x00, 0x40,	/* LD DE, 4000h  ; Length */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h ; Address */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x06, 0x00,		/* LD B, 0 */
	0xCD, 0x09, 0x01,	/* CALL DOS_CLOSE */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
/* Open the SNA file */
	0x01, 0x05, 0x00,	/* LD BC, 5      ; Open file */
	0x11, 0x02, 0x00,	/* LD DE, 2      ; in shared-read mode */
	0x21, 0x04, 0x80,	/* LD HL, fname  ; Filename */
	0xCD, 0x06, 0x01,	/* CALL DOS_OPEN */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x01, 0x00, 0x00,	/* LD BC, 0      ; Load header into bank 2 */
	0x11, 0x1B, 0x00,	/* LD DE, 1Bh    ; Length */
	0x21, 0x00, 0xB0,	/* LD HL, 0B000h ; Header */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x01, 0x00, 0x00,	/* LD BC, 0      ; Load bank 5 into bank 0 */
	0x11, 0x00, 0x40,	/* LD DE, 4000h  ; Length */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h ; Address */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x3A, 0x26, 0x80,	/* LD A, (8026h) ; Pause flag */
	0xE6, 0x02,		/* AND 2 */
	0x20, 0x16,		/* JR Z, +16h */
	0xCD, 0x12, 0x81, 	/* CALL bank0 */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h */
	0x11, 0x00, 0x40,	/* LD DE, 04000h */
	0x01, 0x00, 0x1B,	/* LD BC, 1B00h */
	0xED, 0xB0,		/* LDIR */
	0x3A, 0x1A, 0xB0,	/* LD A, (0B01Ah) ; Border */
	0xD3, 0xFE,		/* OUT (c), A */
	0xCD, 0x00, 0x81, 	/* CALL bank7 */
	0x01, 0x06, 0x00,	/* LD BC, 6      ; Load bank 2 into bank 6 */
	0x11, 0x00, 0x40,	/* LD DE, 4000h  ; Length */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h ; Address */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x01, 0x03, 0x00,	/* LD BC, 3      ; Load bank 0 into bank 3 */
	0x11, 0x00, 0x40,	/* LD DE, 4000h  ; Length */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h ; Address */
	0xCD, 0x12, 0x01,	/* CALL DOS_READ */
	0xD2, 0xEA, 0x80,	/* JP NC, 080EAh ; JP NC, dos_err */
	0x06, 0x00,		/* LD B, 0 */
	0xCD, 0x09, 0x01,	/* CALL DOS_CLOSE */
	0xCD, 0x9C, 0x01,	/* CALL DD_L_OFF_MOTOR */
	0xCD, 0x12, 0x81,	/* CALL bank0 */
	0x3A, 0x26, 0x80,	/* LD A,(8026h) */
	0xE6, 0x01,		/* AND 1 */
	0xCA, 0x26, 0x81,	/* JP Z, 8126h ; Launch */
	0x01, 0x00, 0x00, 	/* LD BC, 0 */
	0x3E, 0x07,		/* LD A, 7 */
	0xD3, 0xFE,		/* OUT (0FEh), A */
	0x3D, 			/* DEC A */
	0x20, 0xFB,		/* JR NZ, -5 */
	0x0B, 			/* DEC BC */
	0xB0,			/* OR B */
	0xB1,			/* OR C */
	0x20, 0xF4,		/* JR NZ, -12 */
	0xC3, 0x26, 0x81,	/* JP 8126h */
/* Handle a +3DOS error */
/* DOS error handler */
	0xF5,			/* PUSH AF */
	0xCD, 0x12, 0x81,	/* CALL 8112h	; Back to BASIC environment */
	0xF1,			/* POP AF */
	0xFE, 0x14,		/* CP 20 */
	0x30, 0x02,		/* JR NC, +2 */
	0xC6, 0x25,		/* ADD A, 25h */
	0xC6, 0x18,		/* ADD A, 18h */
	0x32, 0xFF, 0x80,	/* LD (80FFh), A */
	0xED, 0x7B, 0x24, 0x81,	/* LD SP, (8124h) */
	0xCF, 0x00,		/* RST 8 */
/* Bank7: */
	0x01, 0xFD, 0x7F,	/* LD BC, 7FFDh */
	0xF3,			/* DI */
	0x3A, 0x5C, 0x5B,	/* LD A, (BANKM) */
	0xCB, 0xA7,		/* RES 4, A */
	0xF6, 0x07,		/* OR 7 */
	0x32, 0x5C, 0x5B,	/* LD (BANKM), A */
	0xED, 0x79,		/* OUT (C), A */
	0xFB,			/* EI */
	0xC9,			/* RET */
/* Bank0: */
	0x01, 0xFD, 0x7F,	/* LD BC, 7FFDh */
	0xF3,			/* DI */
	0x3A, 0x5C, 0x5B,	/* LD A, (BANKM) */
	0xCB, 0xE7,		/* SET 4, A */
	0xE6, 0xF8,		/* AND 0F8h */
	0x32, 0x5C, 0x5B,	/* LD (BANKM), A */
	0xED, 0x79,		/* OUT (C), A */
	0xFB,			/* EI */
	0xC9,			/* RET */
/* Old stack pointer */
	0x00, 0x00,
/* Launcher. */
	0xF3,			/* DI */
	0x21, 0x00, 0xC0,	/* LD HL, 0C000h */
	0x11, 0x00, 0x40,	/* LD DE, 4000h */
	0x42, 0x4B,		/* LD BC, DE */
	0xED, 0xB0,		/* LDIR */
	0x2A, 0x02, 0x80,	/* LD HL, (8002h) */
	0x7C,			/* LD A, H */
	0xB5,			/* OR L */
	0x20, 0x11,		/* JR NZ, +17 */
	0x21, 0x00, 0x40,	/* LD HL, 4000h  ;Default to 4000h */
	0x3A, 0x18, 0xB0,	/* LD A, (0B018h); SP high */
	0xFE, 0x80,		/* CP 80h */
	0x30, 0x07,		/* JR NC, +7 */
	0x2A, 0x17, 0xB0,	/* LD HL, (0B017h); SP */
	0x11, 0xF0, 0xFF,	/* LD DE, -16 */
	0x19,			/* ADD HL, DE */
	0xEB,			/* EX DE, HL */
	0xD5,			/* PUSH DE */
	0xDD, 0xE1,		/* POP IX */
	0x21, 0xBE, 0x81,	/* LD HL, 081BEh	;Launcher */
	0x01, 0x0E, 0x00,	/* LD DE, 0Eh */
	0xED, 0xB0,		/* LDIR */
	0x3A, 0x00, 0xB0,	/* LD A, (0B000h)	;I */
	0xED, 0x47,		/* LD I, A */
	0x31, 0x01, 0xB0,	/* LD SP, 0B001h	;HL' */
	0xE1, 0xD1, 0xC1, 0xF1,	/* POP HL,DE,BC,AF */
	0xD9, 			/* EXX */
	0x08,			/* EX AF, AF' */
	0xE1, 0xD1, 0xC1,	/* POP HL,DE,BC */
	0xDD, 0x71, 0x03,	/* LD (IX+3), C */
	0xDD, 0x70, 0x04,	/* LD (IX+4), B */
	0xFD, 0xE1,		/* POP IY */
	0xC1, 			/* POP BC */
	0xC1, 			/* POP BC */
	0x79,			/* LD A, C */
	0xE6, 0x04,		/* AND 4 */
	0x3E, 0xF3,		/* LD A, 0F3h	;DI */
	0x28, 0x02,		/* JR Z, +2 */
	0x3E, 0xFB,		/* LD A, 0FBh	;EI */
	0xDD, 0x77, 0x09,	/* LD (IX+9), A */
	0x78,			/* LD A, B */
	0xED, 0x4F,		/* LD R, A */
	0xC1,			/* POP BC */
	0xDD, 0x71, 0x0C,	/* LD (IX+12),C */
	0xDD, 0x70, 0x0D,	/* LD (IX+13),B */
	0xC1,			/* POP BC */
	0xDD, 0x71, 0x07,	/* LD (IX+7),C */
	0xDD, 0x70, 0x08,	/* LD (IX+8),B */
	0xC1,
	0x78,			/* LD A,B */
	0xD3, 0xFE,		/* OUT (0FEh), A */
	0x79,			/* LD A,C */
	0xB7, 			/* OR A */
	0x20, 0x04,		/* JR NZ, +4 */
	0xED, 0x46,		/* IM 0 */
	0x18, 0x09,		/* JR +9 */
	0x3D,			/* DEC A */
	0x20, 0x04,		/* JR NZ, +4 */
	0xED, 0x56,		/* IM 1 */
	0x18, 0x02,		/* JR +2 */
	0xED, 0x5E,		/* IM 2 */
	0xDD, 0x22, 0xBC, 0x81,	/* LD (81BCh), IX */
	0x01, 0x0C, 0x00,	/* LD BC, 12 */
	0xDD, 0x09,		/* ADD IX, BC */
	0xDD, 0x22, 0xB4, 0x81,	/* LD (81B4h), IX */
	0xDD, 0x2A, 0x11, 0xB0,	/* LD IX, (0B011h) */
	0x31, 0x00, 0x00,	/* LD SP, 0 ; Set earlier */
	0x3E, 0x15,		/* LD A, 15h; Memory paging */
	0x01, 0xFD, 0x1F,	/* LD BC, 1FFDh */
	0xC3, 0x00, 0x00,	/* Set earlier */
/* Final-stage launcher: */
	0xED, 0x79,		/* OUT (C), A */
	0x01, 0x00, 0x00,	/* LD BC, 0 */
	0xF1,			/* POP AF */
	0x31, 0x00, 0x00,	/* LD SP, nn */
	0x00,			/* Becomes EI or DI */
	0xED, 0x45,		/* RETN */
	0x00, 0x00,		/* AF */
};



int main(int argc, char **argv)
{
	dsk_err_t err;
	int n, optend = 0;
	const char *dskfile = NULL;
	const char *snafile = NULL;
	const char *romfile = NULL;
	const char *option, *value;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--"))
		{
			optend = 1;
			continue;
		}
		if (optend || argv[n][0] != '-')
		{
			if      (snafile == NULL) snafile = argv[n];
			else if (dskfile == NULL) dskfile = argv[n];
			else if (romfile == NULL) romfile = argv[n];
			else syntax(AV0);
			continue;
		}
		option = argv[n] + 1;
		if (option[0] == '-') ++option;

		if      (!strcmp (option, "180")) 
		{
			format = 180;
			continue;
		}
		else if (!strcmp (option, "720"))
		{
			format = 720;
			continue;
		}
		else if (!strcmp (option, "nostamps")) 
		{
			timestamp = 0;
			continue;
		}
		else if (!strcmp (option, "cpmonly")) 
		{
			cpmonly = 1;	
			continue;
		}
		else if (!strcmp (option, "dosonly")) 
		{
			dosonly = 1;
			continue;
		}
		else if (!strcmp (option, "pause"))
		{
			pause |= 1;
			continue;
		}
		else if (!strcmp (option, "noscreen"))
		{
			pause |= 2;
			continue;
		}
		else if (!strcmp (option, "usr0"))
		{
			usr0 = 1;
			continue;
		}
		else if (!strcmp (option, "48basic"))
		{
			usr0 = 0;
			continue;
		}

/* Check for --variable=value and -variable value */
		value = strchr(option, '=');
		if (value) ++value;
		else
		{
			++n;
			if (n >= argc) syntax(AV0);
			value = argv[n];
		}
		if (!strncmp(option, "type", 4)) dsktype = value;
		else if (!strncmp(option, "label", 5)) label = value;
		else if (!strncmp(option, "compress", 8)) compress = value;
		else if (!strncmp(option, "base", 4))
		{
			parse_loader(value, AV0);
		}
		else syntax(AV0);
	}
	if (dskfile == NULL || snafile == NULL) 
	{
		syntax(AV0);
	}
	if (cpmonly && dosonly)
	{
		fprintf(stderr, "-cpmonly and -dosonly both present; -cpmonly "
				"takes priority\n");
	}

        dsk_reportfunc_set(report, report_end);

	err = p3fs_mkfs(&gl_fs, dskfile, dsktype, compress, 
			(format == 180) ? boot_180 : boot_720, timestamp);
	if (timestamp && !err && label)
	{
		p3fs_83name(label, fcbname);
		err = p3fs_setlabel(gl_fs, fcbname);
	}
	if (err)
	{
		fprintf(stderr, "Can't create %s: %s\n",
			dskfile, p3fs_strerror(err));
		exit(1);
	}

	if (romfile)
	{
		if (loader != 0 && (loader < 0x4000 || loader > 0x7FF2))
		{
		    fprintf(stderr, "Final-stage loader must be between 16384"
                                    " and 32754 (0x4000-0x7FF2).\n");
		    exit(1);
		}
		/* Write DISK */
		err = put_blob(rsdisk, sizeof(rsdisk), "DISK       ");
		if (err) diewith(err);
		/* Write loader */
		romsna[0x82] = loader & 0xFF;
		romsna[0x83] = (loader >> 8) & 0xFF;
		romsna[0xA6] = pause;
		err = put_blob(romsna, sizeof(romsna), "ROMSNA  ZXB");
		err = put_file(romfile, "CUSTOM  ROM");
		if (err) diewith(err);
	}
	else
	{
		if (loader != 0 && (loader < 0x4000 || loader > 0xBFF2))
		{
		    fprintf(stderr, "Final-stage loader must be between 16384"
                                    " and 49138 (0x4000-0xBFF2).\n");
		    exit(1);
		}
		/* Write DISK */
		err = put_blob(disk, sizeof(disk), "DISK       ");
		if (err) diewith(err);
		/* Write loader */
		snapld[0x82] = usr0 ? 0x10 : 0x30;
		snapld[0x83] = loader & 0xFF;
		snapld[0x84] = (loader >> 8) & 0xFF;
		err = put_blob(snapld, sizeof(snapld), "SNAPLD  ZXB");
	}	

	/* Write snapshot */
	err = put_file(snafile, "SNAPSHOTSNA");
	if (err) diewith(err);
	if (cpmonly == 0 && (format == 180 || format == 720))
	{	
		err = p3fs_dossync(gl_fs, format, dosonly);
		if (err)
		{
			fprintf(stderr, "Cannot do DOS sync on %s: %s\n",
					dskfile, p3fs_strerror(err));
		
		}
	}
	err = p3fs_umount(gl_fs);
	if (err) 
	{
		fprintf(stderr, "Cannot close %s: %s\n",
				dskfile, p3fs_strerror(err));
		return 1;
	}
	return 0;
}
