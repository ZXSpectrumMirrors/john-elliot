/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016-17 John Elliott <seasip.webmaster@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE. */

/* LDBS example software: 
 * Test wrapper for the ldbs_clone() function, which duplicates an LDBS 
 * blockstore.
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldbs.h"

void diewith(const char *s, dsk_err_t err)
{
	switch (err)
	{
		case DSK_ERR_OK:
			break;
		case DSK_ERR_NOTME: 
			fprintf(stderr, "%s: File is not in LDBS disk format\n", s); 
			break;
		case DSK_ERR_SYSERR:
			perror(s);
			break;
		case DSK_ERR_NOMEM:
			fprintf(stderr, "Out of memory\n");
			break;
		default:
			fprintf(stderr, "%s: LibDsk error %d\n", 
				s, err);
			break;
	}
	exit(1);
}


int main(int argc, char **argv)
{
	int ro;
	dsk_err_t err;
	PLDBS source, dest;
	char type[4];

	if (argc < 3)
	{
		fprintf(stderr, "%s: Syntax is %s infile outfile\n",
				argv[0], argv[0]);
		exit(1);
	}
	ro = 1;
	err = ldbs_open(&source, argv[1], type, &ro);
	if (err) diewith(argv[1], err);

	if (memcmp(type, LDBS_DSK_TYPE, 4) &&
	    memcmp(type, LDBS_DSK_TYPE_V1, 4))
	{
		ldbs_close(&source);
		diewith(argv[1], DSK_ERR_NOTME);
	}
	err = ldbs_new(&dest, argv[2], LDBS_DSK_TYPE);
	if (err)
	{
		ldbs_close(&source);
		diewith(argv[2], err);
	}
	err = ldbs_clone(source, dest);
	if (err)
	{
		ldbs_close(&dest);
		ldbs_close(&source);
		diewith("Clone", err);
	}
	err = ldbs_close(&dest);
	if (err)
	{
		ldbs_close(&source);
		diewith(argv[2], err);
	}
	ldbs_close(&source);
	return 0;
}
