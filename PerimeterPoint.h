/*============================================================================
  PerimeterPoint.h

  This is a C++ header file for the PerimeterPoint class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2010

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#ifndef PERIMETERPOINT_H
#define PERIMETERPOINT_H

#include"Point.h"

/*============================================================================
  PerimeterPoint
  This class describes a point on a fire perimeter.
  * Data members are purposely made public for increased speed.
*/
class PerimeterPoint : public Point
{ //PerimeterPoint
public:
  double ROS, FLI, RCX;

  PerimeterPoint();
  PerimeterPoint( double x, double y );
  void   Get( double *A );
  PerimeterPoint& operator=( const PerimeterPoint &RHS );
};//PerimeterPoint

#endif