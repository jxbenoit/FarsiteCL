/*============================================================================
  PerimeterPoint.h

  This is a C++ header file for the PerimeterPoint class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2020

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#ifndef PERIMETERPOINT_H
#define PERIMETERPOINT_H

#include"Point.h"

/*============================================================================
  PerimeterPoint
  This class describes a point on a fire perimeter.
*/
class PerimeterPoint : public Point
{ //PerimeterPoint
  private:
    double ROS, FLI, React;

  public:
    static const int X_VAL = 0;
    static const int Y_VAL = 1;
    static const int ROS_VAL = 2;
    static const int FLI_VAL = 3;
    static const int RCX_VAL = 4;

    PerimeterPoint();
    PerimeterPoint( PerimeterPoint *P );
    PerimeterPoint( double x, double y );
    ~PerimeterPoint();
    void   Get( double *Values );
    double Get( int ValueType );
    double GetFLI() { return FLI; };
    PerimeterPoint& operator=( const PerimeterPoint &RHS );
    void   SetLoc( double X, double Y );
    void   SetCharacteristics( double ROS, double FLI );
    void   SetReact( double React );
    void   Set( PerimeterPoint &From );
    void   Set( PerimeterPoint *From );
    void   Set( double X, double Y, double ROS, double FLI, double React );
    void   Set( double *Values );
    void   SetFLI( double Value );
};//PerimeterPoint

#endif
