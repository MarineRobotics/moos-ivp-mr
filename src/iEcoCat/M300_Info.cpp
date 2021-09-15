/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300_Info.cpp                                   */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#include <cstdlib>
#include <iostream>
#include "M300_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

//----------------------------------------------------------------
// Procedure: showSynopsis

void showSynopsis()
{
  blk("SYNOPSIS:                                                       ");
  blk("------------------------------------                            ");
  blk("  The iM300 application serves as an intermediary between       ");
  blk("  a MOOS community (backseat) and a vehicle (frontseat).        ");
  blk("                                                                ");
  blk("  iM300 performs two essential tasks:                           ");
  blk("                                                                ");
  blk("    1) CONNECT: [FRONTSEAT] <-(IP:TCP)-> [BACKSEAT]             ");
  blk("                                                                ");
  blk("    2) TRANSLATE: [NMEA MESSAGES] <-(iM300)-> [MOOS MESSAGES]   ");
  blk("                                                                ");
}

//----------------------------------------------------------------
// Procedure: showHelpAndExit

void showHelpAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("Usage: iM300 file.moos [OPTIONS]                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("Options:                                                        ");
  mag("  --alias","=<ProcessName>                                      ");
  blk("      Launch iM300 with the given process name                  ");
  blk("      rather than iM300.                                        ");
  mag("  --example, -e                                                 ");
  blk("      Display example MOOS configuration block.                 ");
  mag("  --help, -h                                                    ");
  blk("      Display this help message.                                ");
  mag("  --interface, -i                                               ");
  blk("      Display MOOS publications and subscriptions.              ");
  mag("  --version,-v                                                  ");
  blk("      Display the release version of iM300.                     ");
  blk("                                                                ");
  blk("Note: If argv[2] does not otherwise match a known option,       ");
  blk("      then it will be interpreted as a run alias. This is       ");
  blk("      to support pAntler launching conventions.                 ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showExampleConfigAndExit

void showExampleConfigAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("iM300 Example MOOS Configuration                   ");
  blu("=============================================================== ");
  blk("                                                                ");
  blk("ProcessConfig = iM300                              ");
  blk("{                                                               ");
  blk("  AppTick   = 10                                                ");
  blk("  CommsTick = 10                                                ");
  blk("                                                                ");
  blk("  ip_address     = localhost  // frontseat IP address           ");
  blk("  port_number    = 29500      // port number at IP address      ");
  blk("  gps_prefix     = NAV_       // prepended to MOOS variables    ");
  blk("                                                                ");
  blk("  max_rudder     = 50.0       // Maximum Rudder Angle  [+/- deg]");
  blk("  max_thrust     = 100.0      // Maximum Thrust        [+/- %]  ");
  blk("                                                                ");
  blk("  ignore_msg = $GPGLL                                           ");
  blk("  ignore_msg = $GPGSV, GPVTG, GPZDA                             ");
  blk("                                                                ");
  blk("  max_appcast_events = 8                                        ");
  blk("  max_appcast_run_warnings = 10                                 ");
  blk("}                                                               ");
  blk("                                                                ");
  exit(0);
}


//----------------------------------------------------------------
// Procedure: showInterfaceAndExit

void showInterfaceAndExit()
{
  blk("                                                                ");
  blu("=============================================================== ");
  blu("iM300 INTERFACE                                    ");
  blu("=============================================================== ");
  blk("                                                                ");
  showSynopsis();
  blk("                                                                ");
  blk("SUBSCRIPTIONS:                                                  ");
  blk("------------------------------------                            ");
  blk("  DESIRED_THRUST        (double) Desired thrust            [%]  ");
  blk("  DESIRED_RUDDER        (double) Desired rudder angle      [deg]");
  blk("                                                                ");
  blk("PUBLICATIONS:                                                   ");
  blk("------------------------------------                            ");
  blk("  [gps_prefix]_X        (double)  X-position on local grid [m]  ");
  blk("  [gps_prefix]_Y        (double)  Y-position on local grid [m]  ");
  blk("  [gps_prefix]_LAT      (double)  Latitude                 [deg]");
  blk("  [gps_prefix]_LON      (double)  Longitude                [deg]");
  blk("  [gps_prefix]_SPEED    (double)  Speed                    [m/s]");
  blk("  [gps_prefix]_HEADING  (double)  Heading from true North  [deg]");
  blk("                                                                ");
  blk("  IMU_ROLL              (double)  Roll                     [deg]");
  blk("  IMU_PITCH             (double)  Pitch                    [deg]");
  blk("  IMU_YAW               (double)  Yaw (Heading)            [deg]");
  blk("                                                                ");
  blk("  M300_BATT_VOLTAGE     (double)  Vehicle battery voltage  [V]  ");
  blk("  M300_RAW_NMEA         (string)  Raw NMEA sentences       []   ");
  blk("  M300_THRUST_L         (double)  Motor thrust (left)      [?]  ");
  blk("  M300_THRUST_R         (double)  Motor thrust (right)     [?]  ");
  blk("                                                                ");
  exit(0);
}

//----------------------------------------------------------------
// Procedure: showReleaseInfoAndExit

void showReleaseInfoAndExit()
{
  showReleaseInfo("iM300", "gpl");
  exit(0);
}
