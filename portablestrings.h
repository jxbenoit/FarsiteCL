/*portablestrings.h
  Portable, public domain strupr() & strlwr().
  See LICENSE.TXT file for license information.
*/
#include<ctype.h>
#include<string.h>

char *strupr( char *str );
char *strlwr( char *str );
char *strrev( char *str );

void getNameFromFile( char* source, char* filename, int length );