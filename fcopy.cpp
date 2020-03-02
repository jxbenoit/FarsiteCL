/*fcopy.cpp
  by: Bob Jarvis
  Copy one file to another. Returns the (positive) number of bytes copied,
  or -1 if an error occurred.
 
  See LICENSE.TXT file for license information.
*/
#include<stdio.h>
#include<stdlib.h>
#include"fcopy.h"

#define BUFFER_SIZE 1024

//============================================================================
int fcopy( char *dest, char *source )
{ //fcopy
  FILE   *d, *s;
  char   *buffer;
  size_t incount;
  long   totcount = 0L;

  s = fopen( source, "rb" );
  if( s == NULL ) return -1L;

  d = fopen( dest, "wb" );
  if( d == NULL ) {
    fclose( s );
    return -1L;
  }

  buffer = (char *) malloc( BUFFER_SIZE );
  if( buffer == NULL ) {
    fclose( s );
    fclose( d );
    return -1L;
  }

  incount = fread( buffer, sizeof(char), BUFFER_SIZE, s );

  while( ! feof(s) ) {
    totcount += (long)incount;
    fwrite( buffer, sizeof(char), incount, d );
    incount = fread( buffer, sizeof(char), BUFFER_SIZE, s );
  }

  totcount += (long)incount;
  fwrite( buffer, sizeof(char), incount, d );

  free( buffer );
  fclose( s );
  fclose( d );

  return totcount;
} //fcopy