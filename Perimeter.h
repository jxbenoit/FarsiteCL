/*============================================================================
  Perimeter.h

  This is a C++ header file for the Perimeter class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2020

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#ifndef PERIMETER_H
#define PERIMETER_H

#include"PerimeterPoint.h"

/*============================================================================
  Perimeter
  This class describes a fire perimeter.
*/
class Perimeter
{ //Perimeter
  private:
    PerimeterPoint *Points;
    long Size, MaxSize;

  public:
    Perimeter();
    Perimeter( long Size );
    ~Perimeter();
    Perimeter& operator=( const Perimeter &RHS );
    long GetSize() { return Size; }        //Actual number of pts
    long GetMaxSize() { return MaxSize; }  //Max num pts allowed w/o realloc'g
    double GetValue( long Index, int ValueType );
    PerimeterPoint*  GetPoint( long Index );
    bool DeletePoint( long Index );
    void Set( Perimeter &RHS );
    void Set( Perimeter *RHS );
    bool SetPointLoc( long Index, double X, double Y );
    bool SetPointCharacteristics( long Index, double ROS, double FLI );
    bool SetPointReact( long Index, double React );
};//Perimeter

#endif
