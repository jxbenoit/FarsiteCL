/*fsxwbar.cpp
  Use of Vector Barriers to Fire Spread.
  Copyright 1994, 1995, 1996
  Mark A. Finney, Systems for Environemntal Management
  See LICENSE.TXT file for license information.
*/
#include<math.h>
#include"fsxw.h"
#include"fsglbvar.h"
#include"fsxwbar.h"
#include"globals.h"

//============================================================================
VectorBarrier::VectorBarrier()
{ //VectorBarrier::VectorBarrier
  Barrier = 0;
  BufBarrier = 0;
  NumVertices = 0;
  DistanceResolution = 0;
} //VectorBarrier::VectorBarrier

//============================================================================
VectorBarrier::~VectorBarrier()
{ //VectorBarrier::~VectorBarrier
  FreeBarrier();
} //VectorBarrier::~VectorBarrier

//============================================================================
bool VectorBarrier::AllocBarrier( long VertNumber )
{ //VectorBarrier::AllocBarrier
  if( VertNumber <= 0 ) return false;
  else NumVertices = VertNumber;
  if( Barrier ) free( Barrier );
  Barrier = (double *) malloc( VertNumber * 2 * sizeof(double) );
  memset( Barrier,0x0,VertNumber*2*sizeof(double) );
  if( ! Barrier ) return false;

  return true;
} //VectorBarrier::AllocBarrier

//============================================================================
void VectorBarrier::FreeBarrier()
{ //VectorBarrier::FreeBarrier
  if( Barrier ) free( Barrier );
  if( BufBarrier ) free( BufBarrier );
  Barrier = 0;
  BufBarrier = 0;
} //VectorBarrier::FreeBarrier

//============================================================================
void VectorBarrier::SetBarrierVertex( long VertNumber, double xpt,
                                      double ypt )
{ //VectorBarrier::SetBarrierVertex
  VertNumber *= 2;
  Barrier[VertNumber] = xpt;
  Barrier[VertNumber + 1] = ypt;
} //VectorBarrier::SetBarrierVertex

//============================================================================
bool VectorBarrier::ReDiscretizeBarrier()
{ //VectorBarrier::ReDiscretizeBarrier
  long   i, VertCount, NumNewVertices;
  double xdiff, ydiff, testdist;
  double xpt1, xpt2, ypt1, ypt2, newx, newy;
  double MaxDist, MaxDistSquared;

  MaxDist = GetDistRes() * MetricResolutionConvert();
  MaxDistSquared = MaxDist * MaxDist;

  do {
    NumNewVertices = 0;
    VertCount = 0;

    if( (BufBarrier = (double *) malloc( NumVertices * 4 * sizeof(double)))
        == NULL )
      return false;
    memset( BufBarrier,0x0,NumVertices*4*sizeof(double) );
    xpt1 = Barrier[0];
    ypt1 = Barrier[1];
    for( i = 1; i < NumVertices; i++ ) {
      xpt2 = Barrier[i * 2];
      ypt2 = Barrier[i * 2 + 1];
      xdiff = xpt1 - xpt2;
      ydiff = ypt1 - ypt2;
      testdist = xdiff * xdiff + ydiff * ydiff;
      if( testdist > MaxDistSquared ) {
        BufBarrier[VertCount * 2] = xpt1;
        BufBarrier[VertCount * 2 + 1] = ypt1;
        VertCount++;
        newx = xpt1 - xdiff / 2.0;
        newy = ypt1 - ydiff / 2.0;
        BufBarrier[VertCount * 2] = newx;
        BufBarrier[VertCount * 2 + 1] = newy;
        VertCount++;
        NumNewVertices++;
      }
      else {
        BufBarrier[VertCount * 2] = xpt1;
        BufBarrier[VertCount * 2 + 1] = ypt1;
        VertCount++;
      }
      xpt1 = xpt2;
      ypt1 = ypt2;
    }
    BufBarrier[VertCount * 2] = xpt1;
    BufBarrier[VertCount * 2 + 1] = ypt1;
    VertCount++;
    free( Barrier );
    Barrier = 0;
    AllocBarrier( VertCount );
    for( i = 0; i < VertCount; i++ )
      SetBarrierVertex( i, BufBarrier[i * 2], BufBarrier[i * 2 + 1] );
    free( BufBarrier );
  } while( NumNewVertices > 0 );

  BufBarrier = (double *) malloc( NumVertices * 4 * sizeof(double) );
  memset( BufBarrier,0x0,NumVertices * 4 * sizeof(double) );

  return true;
} //VectorBarrier::ReDiscretizeBarrier

//============================================================================
void VectorBarrier::BufferBarrier( double DistResMult )
{ //VectorBarrier::BufferBarrier
  long   i, j;
  double xpt, ypt, xptl, yptl, xptn, yptn;
  double xdiffl, xdiffn, ydiffl, ydiffn, tempx, tempy;
  double xdiff, ydiff, dist, distl, distn;
  double xbuf1, ybuf1;
  double xbuf2, ybuf2;
  double A1, A2, A3, DR;

  ReDiscretizeBarrier();

  DistanceResolution = GetDistRes() * DistResMult /
  2.0 * MetricResolutionConvert();
  xpt = xptl = Barrier[0];
  ypt = yptl = Barrier[1];
  xptn = Barrier[2];
  yptn = Barrier[3];

  for( i = 1; i <= NumVertices; i++ ) {
    xdiffl = xpt - xptl;
    ydiffl = ypt - yptl;
    xdiffn = xpt - xptn;
    ydiffn = ypt - yptn;
    distl = sqrt( xdiffl * xdiffl + ydiffl * ydiffl );
    distn = sqrt( xdiffn * xdiffn + ydiffn * ydiffn );

    if( distl > 0.0 && distn > 0.0 ) {
      if( distl < distn ) {
        tempx = xpt - xdiffn * distl / distn;
        tempy = ypt - ydiffn * distl / distn;
        xdiff = xptl - tempx;
        ydiff = yptl - tempy;
      }
      else {
        if( distn < distl ) {
          tempx = xpt - xdiffl * distn / distl;
          tempy = ypt - ydiffl * distn / distl;
          xdiff = tempx - xptn;
          ydiff = tempy - yptn;
        }
        else {
          xdiff = xptl - xptn;
          ydiff = yptl - yptn;
        }
      }
    }
    else {
      xdiff = xptl - xptn;
      ydiff = yptl - yptn;
    }

    dist = sqrt( xdiff * xdiff + ydiff * ydiff );
    if( fabs(xdiffl) < 1e-9 && fabs(ydiffl) < 1e-9 ) A1 = 0.0;
    else A1 = atan2( (xdiffl), (ydiffl) );
    if( fabs(xdiff) < 1e-9 && fabs(ydiff) < 1e-9 ) A2 = 0.0;
    else A2 = atan2( (xdiff), (ydiff) );
    A3 = cos( A1 ) * cos( A2 ) + sin( A1 ) * sin( A2 );
    if( fabs(A3) > 1e-2 && distl > 1e-9 )
      DR = DistanceResolution / fabs( A3 );
    else DR = DistanceResolution;
    if( dist == 0.0 ) dist = 1.0;
    xbuf1 = xpt - DR / dist * ydiff;  //Perpendicular to xpt,ypt
    ybuf1 = ypt + DR / dist * xdiff;
    xbuf2 = xpt + DR / dist * ydiff;  //Perpendicular to xpt,ypt
    ybuf2 = ypt - DR / dist * xdiff;

    BufBarrier[(i - 1) * 2] = xbuf2;
    BufBarrier[(i - 1) * 2 + 1] = ybuf2;
    BufBarrier[(NumVertices * 2 - i) * 2] = xbuf1;
    BufBarrier[(NumVertices * 2 - i) * 2 + 1] = ybuf1;

    if( i < NumVertices - 1 ) j = i + 1;
    else j = NumVertices - 1;
    xptl = xpt;
    yptl = ypt;
    xpt = xptn;
    ypt = yptn;
    xptn = Barrier[j * 2];
    yptn = Barrier[j * 2 + 1];
  }
} //VectorBarrier::BufferBarrier

//============================================================================
bool VectorBarrier::ReBufferBarriers()
{ //VectorBarrier::ReBufferBarriers
  //Public access.
  if( DistanceResolution == 0.0 ) return false;

  DiffRes = GetDistRes() - DistanceResolution;

  for( long i = 0; i < GetNumFires(); i++ ) {
    if( GetInout(i) == 3 ) ReBuffer( i );
  }
  DistanceResolution = GetDistRes();

  return true;
} //VectorBarrier::ReBufferBarriers

//============================================================================
void VectorBarrier::ReBuffer( long BarrierNumber )
{ //VectorBarrier::ReBuffer
  //Private access if change in distance resolution, then change barrier with
  //from old one.
  if( DiffRes > 0.0 ) {
    long   i, k;
    double xpt, ypt, xptl, yptl, xptn, yptn;
    double xdiffl, xdiffn, ydiffl, ydiffn, tempx, tempy;
    double xdiff, ydiff, dist, distl, distn;
    double xbuf1, ybuf1;

    NumVertices = GetNumPoints( BarrierNumber );

    xptl = GetPerimeter1Value( BarrierNumber, NumVertices - 1, XCOORD );
    yptl = GetPerimeter1Value( BarrierNumber, NumVertices - 1, YCOORD );
    xpt = GetPerimeter1Value( BarrierNumber, 0, XCOORD );
    ypt = GetPerimeter1Value( BarrierNumber, 0, YCOORD );

    for( i = 0; i < NumVertices; i++ ) {
      if( i < NumVertices - 1 ) k = i + 1;
      else k = 0;
      xptn = GetPerimeter1Value( BarrierNumber, k, XCOORD );
      yptn = GetPerimeter1Value( BarrierNumber, k, YCOORD );
      xdiffl = xpt - xptl;
      ydiffl = ypt - yptl;
      xdiffn = xpt - xptn;
      ydiffn = ypt - yptn;
      distl = sqrt( xdiffl * xdiffl + ydiffl * ydiffl );
      distn = sqrt( xdiffn * xdiffn + ydiffn * ydiffn );

      if( distl > 0.0 && distn > 0.0 ) {
        if( distl < distn ) {
          tempx = xpt - xdiffn * distl / distn;
          tempy = ypt - ydiffn * distl / distn;
          xdiff = xptl - tempx;
          ydiff = yptl - tempy;
        }
        else {
          if( distn < distl ) {
            tempx = xpt - xdiffl * distn / distl;
            tempy = ypt - ydiffl * distn / distl;
            xdiff = tempx - xptn;
            ydiff = tempy - yptn;
          }
          else {
            xdiff = xptl - xptn;
            ydiff = yptl - yptn;
          }
        }
      }
      else {
        xdiff = xptl - xptn;
        ydiff = yptl - yptn;
      }

      dist = sqrt( xdiff * xdiff + ydiff * ydiff );
      if( fabs(xdiff) > 0.0 ) {
        xbuf1 = xpt - DiffRes / dist * ydiff;  //Perpendicular to xpt,ypt
        ybuf1 = ypt + DiffRes / dist * xdiff;
      }
      else {
        xbuf1 = xpt;
        ybuf1 = ypt + DiffRes;
      }
      SetPerimeter1( BarrierNumber, i, xbuf1, ybuf1 );
      xptl = xpt;
      yptl = ypt;
      xpt = xptn;
      ypt = yptn;
    }
  }
} //VectorBarrier::ReBuffer

//============================================================================
bool VectorBarrier::TransferBarrier( long NumFire )
{ //VectorBarrier::TransferBarrier
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwbar:TransferBarrier:1 NumVertices=%ld\n",
            CallLevel, "", NumVertices );

  long   i;
  double xpt, ypt;

  if( NumVertices > 0 ) {
    AllocPerimeter1( NumFire, NumVertices * 2 + 1 );
    SetInout( NumFire, 3 );
    SetNumPoints( NumFire, NumVertices * 2 );
    for( i = 0; i < NumVertices*2; i++ ) {
      xpt = BufBarrier[i * 2];
      ypt = BufBarrier[i * 2 + 1];
      SetPerimeter1( NumFire, i, xpt, ypt );
      SetFireChx( NumFire, i, 0.0, -1.0 );
      SetReact( NumFire, i, 0.0 );
    }
  }
  FreeBarrier();

  if( Verbose > CallLevel )
    printf( "%*sfsxwbar:TransferBarrier:2\n", CallLevel, "" );
  CallLevel--;

  return true;
} //VectorBarrier::TransferBarrier
