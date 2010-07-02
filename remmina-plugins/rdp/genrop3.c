/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

/* Small utility to generate rop3 functions from
   http://msdn.microsoft.com/en-us/library/dd145130%28v=VS.85%29.aspx

   $ gcc -Wall -o genrop3 genrop3.c
   $ ./genrop3
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *rop3_reverse_polish[] =
{
NULL,
"DPSoon",
"DPSona",
"PSon",
"SDPona",
"DPon",
"PDSxnon",
"PDSaon",
"SDPnaa",
"PDSxon",
"DPna",
"PSDnaon",
"SPna",
"PDSnaon",
"PDSonon",
"Pn",
"PDSona",
"DSon",
"SDPxnon",
"SDPaon",
"DPSxnon",
"DPSaon",
"PSDPSanaxx",
"SSPxDSxaxn",
"SPxPDxa",
"SDPSanaxn",
"PDSPaox",
"SDPSxaxn",
"PSDPaox",
"DSPDxaxn",
"PDSox",
"PDSoan",
"DPSnaa",
"SDPxon",
"DSna",
"SPDnaon",
"SPxDSxa",
"PDSPanaxn",
"SDPSaox",
"SDPSxnox",
"DPSxa",
"PSDPSaoxxn",
"DPSana",
"SSPxPDxaxn",
"SPDSoax",
"PSDnox",
"PSDPxox",
"PSDnoan",
"PSna",
"SDPnaon",
"SDPSoox",
"Sn",
"SPDSaox",
"SPDSxnox",
"SDPox",
"SDPoan",
"PSDPoax",
"SPDnox",
"SPDSxox",
"SPDnoan",
"PSx",
"SPDSonox",
"SPDSnaox",
"PSan",
"PSDnaa",
"DPSxon",
"SDxPDxa",
"SPDSanaxn",
"SDna",
"DPSnaon",
"DSPDaox",
"PSDPxaxn",
"SDPxa",
"PDSPDaoxxn",
"DPSDoax",
"PDSnox",
"SDPana",
"SSPxDSxoxn",
"PDSPxox",
"PDSnoan",
"PDna",
"DSPnaon",
"DPSDaox",
"SPDSxaxn",
"DPSonon",
"Dn",
"DPSox",
"DPSoan",
"PDSPoax",
"DPSnox",
"DPx",
"DPSDonox",
"DPSDxox",
"DPSnoan",
"DPSDnaox",
"DPan",
"PDSxa",
"DSPDSaoxxn",
"DSPDoax",
"SDPnox",
"SDPSoax",
"DSPnox",
"DSx",
"SDPSonox",
"DSPDSonoxxn",
"PDSxxn",
"DPSax",
"PSDPSoaxxn",
"SDPax",
"PDSPDoaxxn",
"SDPSnoax",
"PDSxnan",
"PDSana",
"SSDxPDxaxn",
"SDPSxox",
"SDPnoan",
"DSPDxox",
"DSPnoan",
"SDPSnaox",
"DSan",
"PDSax",
"DSPDSoaxxn",
"DPSDnoax",
"SDPxnan",
"SPDSnoax",
"DPSxnan",
"SPxDSxo",
"DPSaan",
"DPSaa",
"SPxDSxon",
"DPSxna",
"SPDSnoaxn",
"SDPxna",
"PDSPnoaxn",
"DSPDSoaxx",
"PDSaxn",
"DSa",
"SDPSnaoxn",
"DSPnoa",
"DSPDxoxn",
"SDPnoa",
"SDPSxoxn",
"SSDxPDxax",
"PDSanan",
"PDSxna",
"SDPSnoaxn",
"DPSDPoaxx",
"SPDaxn",
"PSDPSoaxx",
"DPSaxn",
"DPSxx",
"PSDPSonoxx",
"SDPSonoxn",
"DSxn",
"DPSnax",
"SDPSoaxn",
"SPDnax",
"DSPDoaxn",
"DSPDSaoxx",
"PDSxan",
"DPa",
"PDSPnaoxn",
"DPSnoa",
"DPSDxoxn",
"PDSPonoxn",
"PDxn",
"DSPnax",
"PDSPoaxn",
"DPSoa",
"DPSoxn",
"D",
"DPSono",
"SPDSxax",
"DPSDaoxn",
"DSPnao",
"DPno",
"PDSnoa",
"PDSPxoxn",
"SSPxDSxox",
"SDPanan",
"PSDnax",
"DPSDoaxn",
"DPSDPaoxx",
"SDPxan",
"PSDPxax",
"DSPDaoxn",
"DPSnao",
"DSno",
"SPDSanax",
"SDxPDxan",
"DPSxo",
"DPSano",
"PSa",
"SPDSnaoxn",
"SPDSonoxn",
"PSxn",
"SPDnoa",
"SPDSxoxn",
"SDPnax",
"PSDPoaxn",
"SDPoa",
"SPDoxn",
"DPSDxax",
"SPDSaoxn",
"S",
"SDPono",
"SDPnao",
"SPno",
"PSDnoa",
"PSDPxoxn",
"PDSnax",
"SPDSoaxn",
"SSPxPDxax",
"DPSanan",
"PSDPSaoxx",
"DPSxan",
"PDSPxax",
"SDPSaoxn",
"DPSDanax",
"SPxDSxan",
"SPDnao",
"SDno",
"SDPxo",
"SDPano",
"PDSoa",
"PDSoxn",
"DSPDxax",
"PSDPaoxn",
"SDPSxax",
"PDSPaoxn",
"SDPSanax",
"SPxPDxan",
"SSPxDSxax",
"DSPDSanaxxn",
"DPSao",
"DPSxno",
"SDPao",
"SDPxno",
"DSo",
"SDPnoo",
"P",
"PDSono",
"PDSnao",
"PSno",
"PSDnao",
"PDno",
"PDSxo",
"PDSano",
"PDSao",
"PDSxno",
"DPo",
"DPSnoo",
"PSo",
"PSDnoo",
"DPSoo",
NULL
};

int
main (int argc, char *argv[])
{
    char stack[10][50];
    char buf[50];
    int top;
    int i, j;
    char c;

    printf("static guchar remmina_rop3_00 (guchar p, guchar s, guchar d) { return 0; }\n");
    for (i = 1; rop3_reverse_polish[i]; i++)
    {
        top = -1;
        for (j = 0; (c = rop3_reverse_polish[i][j]); j++)
        {
            switch (c)
            {
            case 'D':
                strcpy (stack[++top], "d");
                break;
            case 'S':
                strcpy (stack[++top], "s");
                break;
            case 'P':
                strcpy (stack[++top], "p");
                break;
            case 'n':
                sprintf (buf, "~(%s)", stack[top]);
                strcpy (stack[top], buf);
                break;
            case 'a':
                sprintf (buf, "(%s & %s)", stack[top], stack[top - 1]);
                top--;
                strcpy (stack[top], buf);
                break;
            case 'o':
                sprintf (buf, "(%s | %s)", stack[top], stack[top - 1]);
                top--;
                strcpy (stack[top], buf);
                break;
            case 'x':
                sprintf (buf, "(%s ^ %s)", stack[top], stack[top - 1]);
                top--;
                strcpy (stack[top], buf);
                break;
            }
        }
        printf ("static guchar remmina_rop3_%02x (guchar p, guchar s, guchar d) { return %s; }\n",
            i, stack[0]);
    }
    printf ("static guchar remmina_rop3_ff (guchar p, guchar s, guchar d) { return 0xff; }\n\n");

    printf ("static RemminaROP3Func remmina_rop3_func[] = {\n");
    for (i = 0; i <= 0xff; i++)
    {
        printf ("    remmina_rop3_%02x,\n", i);
    }
    printf ("    NULL\n};\n");
    return 0;
}

