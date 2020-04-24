/*============================================================================
  Perimeter.cpp

  This is a C++ class file for the Perimeter class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2020

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<cstdlib>
#include<string.h>
#include<iostream>
#include"Perimeter.h"

//============================================================================
Perimeter::Perimeter()
{ //Perimeter::Perimeter
  Size = MaxSize = 0;
  Points = NULL;
} //Perimeter::Perimeter

//============================================================================
Perimeter::Perimeter( long Size )
{ //Perimeter::Perimeter
  Points = new PerimeterPoint [ Size ];
  this->Size = Size;
  this->MaxSize = Size;
} //Perimeter::Perimeter

//============================================================================
Perimeter::~Perimeter()
{ //Perimeter::~Perimeter
  if( Points != NULL ) {
    delete [] Points;
  }
} //Perimeter::~Perimeter

//============================================================================
Perimeter& Perimeter::operator=( const Perimeter &RHS )
{ //Perimeter::operator=
  if( MaxSize < RHS.Size || Points == NULL ) {
    if( Points ) delete [] Points;
    Points = new PerimeterPoint [RHS.Size];
    MaxSize = RHS.Size;
  }

  for( long i = 0; i < RHS.Size; i++ )
    this->Points[i] = RHS.Points[i];
  Size = RHS.Size;

  return *this;
} //Perimeter::operator=

//============================================================================
double Perimeter::GetValue( long Index, int ValueType )
{ //Perimeter::GetValue
  if( Index < 0 || Index >= Size ) return 0;

  return Points[Index].Get( ValueType );
} //Perimeter::GetValue

//============================================================================
PerimeterPoint* Perimeter::GetPoint( long Index )
{ //Perimeter::GetPoint
  if( Index < 0 || Index >= Size ) return NULL;

  PerimeterPoint *p = &Points[Index];

  return p;
} //Perimeter::GetPoint

//============================================================================
//Perimeter::DeletePoint
//Removes a point from the perimeter list, shortens the list by 1,
//and decreases NumPoints.
bool Perimeter::DeletePoint( long Index )
{ //Perimeter::DeletePoint
  if( Index < 0 || Index >= Size ) return false;

  memcpy( &Points[Index], &Points[Index + 1],
          (Size - Index) * sizeof(PerimeterPoint) );
  Size--;

  return true;
} //Perimeter::DeletePoint

//============================================================================
bool Perimeter::SetPointLoc( long Index, double X, double Y )
{ //Perimeter::SetPointLoc
  if( Index < 0 || Index >= Size ) return false;

  Points[Index].SetLoc( X, Y );
  return true;
} //Perimeter::SetPointLoc

//============================================================================
bool Perimeter::SetPointCharacteristics( long Index, double ROS, double FLI )
{ //Perimeter::SetPointCharacteristics
  if( Index < 0 || Index >= Size ) return false;

  Points[Index].SetCharacteristics( ROS, FLI );
  return true;
} //Perimeter::SetPointCharacteristics

//============================================================================
bool Perimeter::SetPointReact( long Index, double React )
{ //Perimeter::SetPointReact
  if( Index < 0 || Index >= Size ) return false;

  Points[Index].SetReact( React );
  return true;
} //Perimeter::SetPointReact

//============================================================================
void Perimeter::Set( Perimeter &RHS )
{ //Perimeter::Set
  if( MaxSize < RHS.Size || Points == NULL ) {
    if( Points ) delete [] Points;
    Points = new PerimeterPoint [RHS.Size];
    MaxSize = RHS.Size;
  }

  for( long i = 0; i < RHS.Size; i++ )
    this->Points[i] = RHS.Points[i];

  Size = RHS.Size;
} //Perimeter::Set

//============================================================================
void Perimeter::Set( Perimeter *RHS )
{ //Perimeter::Set
  if( MaxSize < RHS->Size || Points == NULL ) {
    if( Points ) delete [] Points;
    Points = new PerimeterPoint [RHS->Size];
    MaxSize = RHS->Size;
  }

  for( long i = 0; i < RHS->Size; i++ )
    this->Points[i] = RHS->Points[i];

  Size = RHS->Size;
} //Perimeter::Set
