/*globals.cpp -- Jun 2013 -- J.Benoit
  Code that varies with the operating system / compiler should be put in here.

  At compile time, be sure to specify a variable named VISUAL_STUDIO, DEV_C,
  or OS_X (whichever is appropriate). For example, for using the c++ compiler
  in Mac OS X, the option '-D OS_X' should do the trick.
*/
#ifndef GLOBALS_CPP
#define GLOBALS_CPP

//###########################################################################
#ifdef VISUAL_STUDIO       //______________________________ For Visual Studio
#include<io.h>         //VC++: for _access(), _chmod()
#include<direct.h>     //VC++: for _getcwd()

const char DIR_DELIMITER = '\\';

#elif defined( DEV_C )     //______________________________________ For Dev C
#include<io.h>
#include<direct.h>

const char DIR_DELIMITER = '\\';

#elif defined( OS_X ) //________________________________________ For Mac OS X
#include<unistd.h>     //c++: for access(), chdir()

const char DIR_DELIMITER = '/';

#endif
//###########################################################################

#include<stdio.h>    //for NULL
#include<string.h>   //strlen, strcpy
#include<sys/stat.h> //VC++: for _S_IREAD, _S_IWRITE (used in _chmod())
                     //c++: for S_IRUSR, S_IRGRP, S_IROTH, S_IWUSR, S_IWGRP
                     //         S_IWOTH (used in chmod())
#include"globals.h"

int Verbose = 0;
int CallLevel = 0;
VerboseOut VOut;  //Verbose output stream

/*============================================================================
  Exists
  Returns true if a file exists, false otherwise.
  In C/C++, there are a few functions that can do this, but most are
  compiler-specific. Here are some:
    stat()
    lstat()
    access()
    _access()  - Visual C++ specific
  Another method is to use fopen() on the file & check if an error occurs.
*/
bool Exists( char *Filename )
{ //Exists
//###########################################################################
#ifdef VISUAL_STUDIO             //________________________ For Visual Studio
  if( _access(Filename,0) == 0 ) return true;
#elif defined( DEV_C )   //________________________________________ For Dev C
  if( access(Filename,F_OK) == 0 ) return true;
#elif defined( OS_X )   //______________________________________ For Mac OS X
  if( access(Filename,F_OK) == 0 ) return true;
#else                    //__________________________________ Everything else 
  if( _access(Filename,0) == 0 ) return true;
#endif
//###########################################################################
  else return false;
} //Exists

/*============================================================================
  IsReadable
  Returns true if a file can be read, false otherwise.
  In C/C++, there are a few functions that can do this, but most are
  compiler-specific. Here are some:
    access()
    _access()  - Visual C++ specific
*/
bool IsReadable( char *Filename )
{ //IsReadable
//###########################################################################
#ifdef VISUAL_STUDIO             //________________________ For Visual Studio
  if( _access(Filename,4) == 0 ) return true;
#elif defined( DEV_C )   //________________________________________ For Dev C
  if( access(Filename,R_OK) == 0 ) return true;
#elif defined( OS_X )   //______________________________________ For Mac OS X
  if( access(Filename,R_OK) == 0 ) return true;
#else   //___________________________________________________ Everything else
  if( _access(Filename,4) == 0 ) return true;
#endif
//###########################################################################
  else return false;
} //IsReadable

/*============================================================================
  IsWritable
  Returns true if a file can be written, false otherwise.
  In C/C++, there are a few functions that can do this, but most are
  compiler-specific. Here are some:
    access()
    _access()  - Visual C++ specific
*/
bool IsWritable( char *Filename )
{ //IsWritable
//###########################################################################
#ifdef VISUAL_STUDIO           //__________________________ For Visual Studio
  if( _access(Filename,2) == 0 ) return true;
#elif defined( DEV_C )     //______________________________________ For Dev C
  if( access(Filename,W_OK) == 0 ) return true;
#elif defined( OS_X )   //______________________________________ For Mac OS X
  if( access(Filename,W_OK) == 0 ) return true;
#else   //___________________________________________________ Everything else
  if( _access(Filename,2) == 0 ) return true;
#endif
//###########################################################################
  else return false;
} //IsWritable

/*============================================================================
  GetCurDir
  This writes the current working directory into the given a string buffer.
  It also returns a pointer to the buffer.

  A NULL pointer is returned if the directory string is longer than MaxLen or
  if the function can't figure out what the directory delimiter is
  (usually '/' or '\').

  There are some compiler-specific functions that do this:
    getcwd()
    _getcwd()  - Visual C++ specific
*/
char* GetCurDir( char *Buf, int MaxLen )
{ //GetCurDir
  if( ! getcwd(Buf, MaxLen - 1) )
    return NULL;  //Dir string is too long

  //Append an appropriate trailing directory delimiter ('\' or '/').
  int len = strlen( Buf );
  if( DIR_DELIMITER == '/' || DIR_DELIMITER == '\\' )
    Buf[ len ] = DIR_DELIMITER; 
  else {
    int i = 0;
    while( Buf[i] != 0 && Buf[i] != '/' && Buf[i] != '\\' ) i++;

    if( Buf[i] == 0 )
      return NULL; //Can't determine dir delimiter

    Buf[ len ] = Buf[i];
  }

  Buf[ len + 1 ] = 0;

  return Buf;
} //GetCurDir

/*============================================================================
  ChangeDir
  There is no standard C++ function call for this, from what I've read so far
  e.g. see http://www.informit.com/guides/content.aspx?g=cplusplus&seqNum=245
  (Jul 2009). There are some compiler-specific libraries with functions to do
  this:
    chdir()
    _chdir()  - Visual C++ specific
*/
bool ChangeDir( char *Dir )
{ //ChangeDir
//###########################################################################
#ifdef VISUAL_STUDIO             //________________________ For Visual Studio
  if( _chdir(Dir) == 0 ) return true;
#elif defined( DEV_C )     //______________________________________ For Dev C
  if( chdir(Dir) == 0 ) return true;
#elif defined( OS_X )   //______________________________________ For Mac OS X
  if( chdir(Dir) == 0 ) return true;
#else   //___________________________________________________ Everything else
  if( chdir(Dir) == 0 ) return true;
#endif
//###########################################################################
  else return false;  //Directory path not found
} //ChangeDir

/*============================================================================
  SetFileMode
  In C/C++, there are a few functions that can do this, but most are
  compiler-specific. Here are some:
    _chmod()  - Visual C++ specific
*/
void SetFileMode( char *Filename, int Mode )
{ //SetFileMode
  int new_mode = 0;
//###########################################################################
#ifdef VISUAL_STUDIO             //________________________ For Visual Studio
  if( Mode & FILE_MODE_READ ) new_mode |= _S_IREAD;
  if( Mode & FILE_MODE_WRITE ) new_mode |= _S_IWRITE;
  _chmod( Filename, new_mode );
#elif defined( DEV_C )          //_________________________________ For Dev C
  if( Mode & FILE_MODE_READ ) new_mode |= _S_IREAD;
  if( Mode & FILE_MODE_WRITE ) new_mode |= _S_IWRITE;
  _chmod( Filename, new_mode );
#elif defined( OS_X )    //_____________________________________ For Mac OS X
  if( Mode & FILE_MODE_READ ) new_mode |= ( S_IRUSR | S_IRGRP | S_IROTH );
  if( Mode & FILE_MODE_WRITE ) new_mode |= ( S_IWUSR | S_IWGRP | S_IWOTH );
  chmod( Filename, new_mode );
#endif
//###########################################################################
} //SetFileMode

#endif
