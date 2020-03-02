/*portablestrings.cpp
  Portable, public domain strupr() & strlwr().
  See LICENSE.TXT file for license information.
 */
#include<ctype.h>
#include<string.h>

//============================================================================
char *strupr( char *str )
{ //strupr
  char *string = str;

  if( str )
    for( ; *str; ++str ) *str = toupper( *str );

  return string;
} //strupr

//============================================================================
char *strlwr( char *str )
{ //strlwr
  char *string = str;

  if( str )
    for( ; *str; ++str ) *str = tolower( *str );

  return string;
} //strlwr

/*============================================================================
  strrev
  Reverse a string in place.
  public domain by Bob Stout.
*/
char *strrev( char *str )
{ //strrev
  char *p1, *p2;

  if( ! str || ! *str ) return str;
  for( p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2 ) {
    *p1 ^= *p2;
    *p2 ^= *p1;
    *p1 ^= *p2;
  }
  return str;
} //strrev

//============================================================================
void getNameFromFile( char* source, char* filename, int length )
{ //getNameFromFile
  strcpy( filename, strrchr(source,'/') );
} //getNameFromFile