/*============================================================================
  VerboseOut.h

  A class that allows indented printing of statements, mainly for debugging.
  ============================================================================
*/
#include<iostream>
#include<string>
#include"VerboseOut.h"

using namespace std;

VerboseOut::VerboseOut()
{ //VerboseOut::VerboseOut
  Verbosity = 0;
  CallLevel = 0;
  Indent = true;
  S = "";
} //VerboseOut::VerboseOut

VerboseOut::VerboseOut( int V )
{ //VerboseOut::VerboseOut
  Verbosity = V;
  CallLevel = 0;
  Indent = true;
  S = "";
} //VerboseOut::VerboseOut

bool VerboseOut::CanPrint()
{ //Verbose::CanPrint
  bool YN = ( Verbosity > CallLevel ) ? true : false;
  return YN;
} //Verbose::CanPrint

VerboseOut& operator<<( VerboseOut& VO, const string& Msg )
{ //VerboseOut::operator<<
  int len = Msg.length();
  int from = 0;

  for( int i = 0; i < len; i++ ) {
    if( Msg[i] == '\n' ) {
      if( i >= from || VO.S.length() ) {
        cout << string( VO.CallLevel, '.' );
      }
      cout << VO.S;
      cout << Msg.substr( from, i-from ) << endl;
      from = i + 1;
      VO.S = "";
    }
  }

  if( len > from ) {
    VO.S += Msg.substr( from, len - from );
    VO.Indent = false;
  }

  return VO;
} //VerboseOut::operator<<

VerboseOut& operator<<( VerboseOut& VO, const int N )
{ //VerboseOut::operator<<
/*
  if( VO.Indent ) {
    VO.S += string( VO.CallLevel, '.' );
    VO.Indent = false;
  }
*/
  VO.S += to_string( N );

  return VO;
} //VerboseOut::operator<<
