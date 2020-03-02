/*fsxwinpt.h
  See LICENSE.TXT file for license information.
*/
#include"fsglbvar.h"

#ifndef FSXWINPT_H
#define FSXWINPT_H

//============================================================================
struct FarInputs
{ //FarInputs
private:
  //VC++ 6 doesn't like static consts declared as below. This is a reported
  //bug. See http://support.microsoft.com/kb/241569.
  //static const int FILE_LEN = 256;
  enum{ FILE_LEN = 256 };  //Workaround

public:
  char LandscapeFile[FILE_LEN];
  char ProjectFile[FILE_LEN];
  char AdjustmentFile[FILE_LEN];
  char FuelMoistureFile[FILE_LEN];
  char ConvertFile[FILE_LEN];
  char FuelModelFile[FILE_LEN];
  char WeatherFile[5][FILE_LEN];
  char WindFile[5][FILE_LEN];
  char BurnPeriod[FILE_LEN];
  char CoarseWoody[FILE_LEN];
  char MBLand[64], MBWtr[5][64], MBWnd[5][64];
  char MBAdj[64], MBMois[64], MBConv[64], MBFuelMods[64], MBBpd[64],
       MBCwd[64];
  bool LandID, WtrID, WndID, AdjID, FuelMoisID, BpdID, CwdID;
  char pathdiv[1];
  bool inited;
  bool m_shapeFileInited;
  bool m_gridEastNorthSet;
  bool m_gridRowSet;
  char m_GridRow[256];
  char m_eastNorth[256];
  int  m_BurnPeriodIsHere;
  int  version;
  int  m_dcount;
  long GridInterval;
  FILE* CurrentFile;

  FarInputs();
  void ResetData();
  void InitializeLandscapeFile();
  bool LoadWeatherData( long FileNum );
  bool LoadWindData( long FileNum );
  bool LoadAdjustmentFile();
  bool LoadFuelMoistureFile();
  bool LoadCustomModelFile();
  bool LoadConversionFile();
  bool LoadBurnPeriodFile();
  bool LoadCoarseWoodyFile( char* ErrMsg );
  bool TestProjectFileVersion();
  bool OpenVersion1ProjectFile();
  bool OpenVersion2ProjectFile();
  bool OpenVersion4ProjectFile();
  void MakeProjectFile();
  void WriteWeatherWindProject();
  bool TestForAtmWeather();
  bool TestForAtmWinds();

  //Old load project file replacement functions.
  bool intializeDir( char* Name );
  bool setWeatherFile( int WeatherFileNum, const char* weatherFileName );
  bool setWindFile( int WindFileNum, const char* WindFileName );
  bool setLandscapeFile( const char* value );
  void initWindAndWeather();
  bool loadUpAdjustmentsFile( const char* adjFileName );
  bool loadUpMoistureFile( const char* moistureFile );
  bool loadUpFuelModelFile( const char* FileName );
  bool loadUpConversionFile( const char* ConvertFile );
  bool loadUpCoarseWoodyFile( const char* CoarseWoody );
  bool loadUpBurnPeriodFile( const char* FileName1);
  void saveGridEastNorth( char* eastNorth );
  void saveGridRow( char* GridRow );
  bool loadUpEastNorth( char* eastNorth, char* GridRow );
  bool loadUpLandscapeTheme( const char* FileName1 );
  void initShapeFileLoad();
  bool loadUpShapeFile( char* shapefile );
  bool loadUpStoredVector( char* storedVectorFile );
  bool loadUpSetDirection( char* value,char direction );
  bool loadUpCanopyCheck( char* value );

  bool SaveWtr( long Station );
  bool SaveWnd( long Station );
  bool SaveAdj();
  bool SaveFms();
  bool SaveCnv();
  bool SaveFmd();
  bool SaveCwd();
};//FarInputs

#endif //FSXWINPT_H