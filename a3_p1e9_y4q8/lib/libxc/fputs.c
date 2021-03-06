/* fputs.c - fputs */


#include <xeroslib.h>
#include "xerosPrivLib.h"

/*------------------------------------------------------------------------
 *  fputs  --  write a null-terminated string to a device (file)
 *------------------------------------------------------------------------
 */


int fputs(register char *s, register int dev)
{
	register int r = 0;
	register int c;

	while ((c = *s++))
                r = putc(dev, c);
	return r;
}
