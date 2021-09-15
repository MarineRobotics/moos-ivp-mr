/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300.cpp                                        */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#include "MBUtils.h"
#include "LatLonFormatUtils.h"
#include "M300.h"

using namespace std;

//---------------------------------------------------------
// Constructor

M300::M300()
{
  // Configuration variables  (overwritten by .moos params)
  // TODO: change these max variables to heading and speed
  m_max_rudder   = 30.0;        // default MAX_RUDDER (+/-)
  m_max_thrust   = 100.0;       // default MAX_THRUST (+/-)
  m_drive_mode   = "normal";    // default DRIVE_MODE ("normal"|"aggro")

  m_ivp_allstop      = true;

  // Stale Message Detection
  m_stale_check_enabled = false;
  m_stale_mode          = false;
  m_stale_threshold     = 1.5;
  m_count_stale         = 0;
  m_tstamp_des_rudder   = 0;
  m_tstamp_des_thrust   = 0;

  m_num_satellites      = 0;
  m_batt_voltage        = 0;
  m_bad_nmea_semantic   = 0;

  m_nav_x   = -1;
  m_nav_y   = -1;
  m_nav_hdg = -1;
  m_nav_spd = -1;

  use_imu_heading       = false;

  m_fs_state = UNDEFINED;
}

//---------------------------------------------------------
// Destructor

M300::~M300()
{
  m_ninja.closeSockFDs();
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool M300::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  //------------------------------------------------------
  // HANDLE PARAMETERS IN .MOOS FILE ---------------------
  //------------------------------------------------------
  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for (p = sParams.begin(); p != sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if((param == "port") && isNumber(value)) {
      int port = atoi(value.c_str());
      handled = m_ninja.setPortNumber(port);
    }
    else if(param == "ip_addr")
      handled = m_ninja.setIPAddr(value);
    else if(param == "comms_type")
      handled = m_ninja.setCommsType(value);
    else if(param == "stale_thresh")
      handled = setPosDoubleOnString(m_stale_threshold, value);
    else if(param == "max_rudder")
      handled = m_thrust.setMaxRudder(value);
    else if(param == "max_thrust")
      handled = m_thrust.setMaxThrust(value);
    // TODO: should we use something similar to drive mode
    //       to set the sailing mode, therefore set in mission file
    else if(param == "drive_mode")
      handled = m_thrust.setDriveMode(value);
    else if(param == "ignore_msg") 
      handled = handleConfigIgnoreMsg(value);
    else if(param == "use_imu_heading") 
      handled = handleConfigUseImuHdg(value);

    if(!handled){
      reportUnhandledConfigWarning(orig);
      list<string> warnings = m_thrust.getWarnings();
      while (!warnings.empty()){
        reportConfigWarning(warnings.front());
        warnings.pop_front();
      }
    }
  }
  
  // Init Geodesy 
  GeodesySetup();
  
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool M300::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void M300::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("IVPHELM_ALLSTOP", 0);
  Register("DESIRED_THRUST",  0);
  Register("DESIRED_RUDDER",  0);
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool M300::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    double mtime  = msg.GetTime();
    string key    = msg.GetKey();
    double dval   = msg.GetDouble();
    string sval   = msg.GetString();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity(); 
    string msrc  = msg.GetSource();  
#endif
    // TODO: change this to messages expected from MR moos
    // Once these have been changec
    // This probbly means: 
    // - sail mode (powerboat - hybrid - purist)
    // - desired heading
    // - desired prop power 
    if(key == "IVPHELM_ALLSTOP")
      m_ivp_allstop = (toupper(sval) != "CLEAR");
    // OLD M300 MESSAGES
    // else if(key == "DESIRED_RUDDER") {
    //   m_tstamp_des_rudder = mtime;
    //   m_thrust.setRudder(dval);
    // }
    // else if(key == "DESIRED_THRUST") {
    //   m_tstamp_des_thrust = mtime;
    //   m_thrust.setThrust(dval);
    // }

    // NEW Marine Robotics messages
    // TODO: Figure out exact name of messages sent by our MOOS mission
    else if(key == "DESIRED_HEADING"){
      m_tstamp_des_heading = mtime;
      m_des_heading = dval;
    }
    else if(key == "DESIRED_SPEED"){
      // TODO: moos missoin should specify units here instead of hardcoding
      m_tstamp_des_speed = mtime;
      m_des_speed = dval;
      m_des_speed_unit = "MS";
    }
    else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
      reportRunWarning("Unhandled Mail: " + key);
  }
  return(true);
}


//---------------------------------------------------------
// Procedure: Iterate()

bool M300::Iterate()
{
  AppCastingMOOSApp::Iterate();
  
  // Part 1: Check for allstop or staleness
  checkForStalenessOrAllStop();

  // Part 2: Connect if needed, and write/read from socket
  if(m_ninja.getState() != "connected")
    m_ninja.setupConnection();

  if(m_ninja.getState() == "connected") {
    checkFrontSeatState();
    sendMessagesToSocket();
    readMessagesFromSocket();
  }

  // Part 3: Get Appcast events from ninja and report them
  reportWarningsEvents();
  AppCastingMOOSApp::PostReport();
  return(true);
}


//---------------------------------------------------------
// Procedure: GeodesySetup
//   Purpose: Initialize geodesy object with lat/lon origin.
//            Used for LatLon2LocalUTM conversion.

bool M300::GeodesySetup()
{
  double LatOrigin = 0.0;
  double LonOrigin = 0.0;

  // Get Latitude Origin from .MOOS Mission File
  bool latOK = m_MissionReader.GetValue("LatOrigin", LatOrigin);
  if(!latOK) {
    reportConfigWarning("Latitude origin missing in MOOS file.");
    return(false);
  }

  // Get Longitude Origin from .MOOS Mission File
  bool lonOK = m_MissionReader.GetValue("LongOrigin", LonOrigin);
  if(!lonOK){
    reportConfigWarning("Longitude origin missing in MOOS file.");
    return(false);
  }

  // Initialise CMOOSGeodesy object
  bool geoOK = m_geodesy.Initialise(LatOrigin, LonOrigin);
  if(!geoOK) {
    reportConfigWarning("CMOOSGeodesy::Initialise() failed. Invalid origin.");
    return(false);
  }

  return(true);
}

//---------------------------------------------------------
// Procedure: sendMessagesToSocket()

void M300::sendMessagesToSocket()
{
  // Update differential thrust values
  //m_thrust.calcDiffThrust(); 
  //double thrustL = m_thrust.getThrustLeft();
  //double thrustR = m_thrust.getThrustRight();
  
  // Send the primary PYDIR front seat command
  //string msg = "PYDIR,";
  // msg += doubleToStringX(thrustL,1) + ",";
  // msg += doubleToStringX(thrustR,1);
  // msg = "$" + msg + "*" + checksumHexStr(msg) + "\r\n";

  // Send primary MRCMD front seat command
  string msg = "MRCMD,";
  msg += doubleToStringX(m_curr_time,1) + ",";
  msg += doubleToStringX(m_des_heading,1) + ",";
  msg += "T,";
  msg += doubleToStringX(m_des_speed,1) + ",";
  msg += m_des_speed_unit;
  msg = "$" + msg + "*" + checksumHexStr(msg) + "\r\n";

  m_ninja.sendSockMessage(msg);
  
  // Publish command to MOOSDB for logging/debugging
  // TODO: check out how notify works, notify cmd command sent
  //Notify("PYDIR_THRUST_L", thrustL);
  //Notify("PYDIR_THRUST_R", thrustR);
}

//---------------------------------------------------------
// Procedure: readMessagesFromSocket()
//      Note: Messages returned from the SockNinja have been
//            confirmed to be valid NMEA format and checksum

void M300::readMessagesFromSocket()
{
  list<string> incoming_msgs = m_ninja.getSockMessages();
  list<string>::iterator p;
  for(p=incoming_msgs.begin(); p!=incoming_msgs.end(); p++) {
    string msg = *p;
    msg = biteString(msg, '\r'); // Remove CRLF
    Notify("IM300_RAW_NMEA", msg);

    bool handled = false;
    if(m_ignore_msgs.count(msg.substr (0,6)) >= 1) 
      handled = true;
    // else if(strBegins(msg, "$GPRMC"))
    //   handled = handleMsgGPRMC(msg);
    // else if(strBegins(msg, "$GPGGA"))
    //   handled = handleMsgGPGGA(msg);
    // else if(strBegins(msg, "$CPNVG")){
    //   if (use_imu_heading)
    //     handled = handleMsgCPNVG_heading(msg);
    // }
    // else if(strBegins(msg, "$CPRBS"))
    //   handled = handleMsgCPRBS(msg);

    // Marine robotics handlers
    else if(strBegins(msg, "$MRINF"))
      handled = handleMsgMRINF(msg);
    else if(strBegins(msg, "$MRHDG"))
      handled = handleMsgMRHDG(msg);
    else if(strBegins(msg, "$MRSPW"))
      handled = handleMsgMRSPW(msg);
    else if(strBegins(msg, "$MRGNS"))
      handled = handleMsgMRGNS(msg);
    else if(strBegins(msg, "$MRFSS"))
      handled = handleMsgMRFSS(msg);
    else
      reportBadMessage(msg, "Unknown NMEA Key");
            
    if(!handled)
      m_bad_nmea_semantic++;
  }
}

//---------------------------------------------------------
// Procedure: handleConfigIgnoreMsg()
//  Examples: ignore_msg = $GPGLL
//            ignore_msg = $GPGLL, GPGSV, $GPVTG

bool M300::handleConfigIgnoreMsg(string str)
{
  bool all_ok = true;
  
  vector<string> msgs = parseString(str, ',');
  for(unsigned int i=0; i<msgs.size(); i++) {
    string msg = stripBlankEnds(msgs[i]);
    // Check if proper NMEA Header
    if((msg.length() == 6) && (msg.at(0) = '$'))
      m_ignore_msgs.insert(msg);
    else
      all_ok = false;
  }

  return(all_ok);
}

//---------------------------------------------------------
// Procedure: handleConfigUseImuHdg()
//  Examples: 

bool M300::handleConfigUseImuHdg(string str)
{
  bool all_ok = true;
  
  if(atof(str.c_str())){
      use_imu_heading = true;
  }
  return(all_ok);
}

//---------------------------------------------------------
// Procedure: checkForStalenessOrAllStop()
//   Purpose: If DESIRED_RUDDER or _THRUST commands are stale,
//            set local desired_rudder/thrust to zero.
//            If an all-stop has been posted, also set the
//            local desired_rudder/thrust vals to zero.

void M300::checkForStalenessOrAllStop()
{
  if(m_ivp_allstop) {
    //m_thrust.setRudder(0);
    //m_thrust.setThrust(0);
    m_des_speed = 0;
    return;
  }

  // If not checking staleness, ensure stale mode false, return.
  // TODO: stale check is disabled by default
  if(!m_stale_check_enabled) {
    m_stale_mode = false;
    return;
  }
  //double lag_rudder = m_curr_time - m_tstamp_des_rudder;
  //double lag_thrust = m_curr_time - m_tstamp_des_thrust;

  double lag_heading = m_curr_time - m_tstamp_des_heading;
  double lag_speed = m_curr_time - m_tstamp_des_speed;

  //bool stale_rudder = (lag_rudder > m_stale_threshold);
  //bool stale_thrust = (lag_thrust > m_stale_threshold);

  bool stale_heading = (lag_heading > m_stale_threshold);
  bool stale_speed = (lag_speed > m_stale_threshold);

  
  if(stale_heading)
    m_count_stale++;
  if(stale_speed)
    m_count_stale++;

  bool stale_mode = false;
  if(stale_heading || stale_speed) {
    //m_thrust.setRudder(0);
    //m_thrust.setThrust(0);
    // TODO: it would be cool to make the boat
    //       weatherhelm here instead of just setting speed to 0
    //       same goes for all stop
    m_des_speed = 0;
    stale_mode = true;
  }

  // Check new stale_mode represents a change from previous
  if(stale_mode && !m_stale_mode) 
    reportRunWarning("Stale Command Detected: Stopping Vehicle");
  if(!stale_mode && m_stale_mode) 
    retractRunWarning("Stale Command Detected: Stopping Vehicle");

  m_stale_mode = stale_mode;
}

//---------------------------------------------------------
// START new MR message handlers

//---------------------------------------------------------
// Procedure: checkFrontSeatState()
//   Purpose: Handle the current frontseat state appropriately. If:
//            - "MANUAL": publish all_stop.
//            - "IDLE" and not stale: send start command to front seat
//            - "PAYLOAD": Continue

void M300::checkFrontSeatState()
{
  switch(m_fs_state)
  {
    case MANUAL:
      if(!m_ivp_allstop)
      {
        // TODO: Test that this is the correct way to do this 
        // TODO: Make sure M300 then detects the allstop and reacts to it
        Notify("IVPHELM_ALLSTOP", "Frontseat has taken control");
      }
      break;
    case IDLE:
      // code block
      if(!m_stale_mode)
      {
        // Publish start command to frontseat. This changes the state to "PAYLOAD "
        // And allows the frontseat to start accepting backseat commands
        string msg = "MRCTL,START,";
        msg += doubleToStringX(m_curr_time, 2) + ",";
        msg += "0";
        msg = "$" + msg + "*" + checksumHexStr(msg) + "\r\n";

        m_ninja.sendSockMessage(msg);

        // TODO: Should we try to move this to the "send messages to socket function"
        //       So that all socket operations are grouped together?
        //       Maybe we can use a message queue?

        // TODO: Should I publish this command to MOOSDB similar to "sendMessagesToSocket"?
      }
      break;
  }
}




//----------------------------------------------------------
// Procedure: handleMsgMRINF()
//      Note:
//   Example: $MRINF,213,T,1.3,MS,44.0888889,9.84861111*7F

// 0 $MRINF                Sentence type
// 1 [Heading]             Current heading in degrees
// 2 [Reference]           Heading reference; “T”-true, “M”-magnetic, “U”-unspecified
// 3 [Water speed]         Speed over water
// 4 [Water speed unit]    “KT”-knots, “MS”-meters/second, “KM”-km/h, “U”-unspecified
// 5 Latitude              decimal degrees
// 6 Longitude             decimal degrees

bool M300::handleMsgMRINF(string msg)
{
  if(!strBegins(msg, "$MRINF,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 7)
    return(reportBadMessage(msg, "Wrong field count"));

  string str_lat = flds[5];
  string str_lon = flds[6];
  string str_hdg = flds[1];
  string str_vw  = flds[3];
  string str_v_unit = flds[4];

  double dbl_lat = atof(str_lat.c_str());
  double dbl_lon = atof(str_lon.c_str());
  
  double x, y;
  bool ok = m_geodesy.LatLong2LocalGrid(dbl_lat, dbl_lon, y, x);
  if(ok) {
    m_nav_x = x;
    m_nav_y = y;
    Notify("NAV_X", x, "GPRMC");
    Notify("NAV_Y", y, "GPRMC");
  }

  double dbl_vw = atof(str_vw.c_str());
  double dbl_mps;
  // Convert water speed to meters per second
  // TODO: turn this into a reusable "convert_mps" functions
  if(str_v_unit == "KT")
    dbl_mps = dbl_vw * 0.514444;
  else if(str_v_unit == "KM")
    dbl_mps = dbl_vw * 0.277778;
  else if(str_v_unit == "MS")
    dbl_mps = dbl_vw;
  else
  {
    // TODO: post warning
    dbl_mps = dbl_vw;
  }

  m_nav_spd = dbl_mps;

  // TODO: convert magnetic to true if heading is magnetic
  double dbl_hdg = atof(str_hdg.c_str());
  m_nav_hdg = dbl_hdg;

  return(true);
}

//----------------------------------------------------------
// Procedure: handleMsgMRHDG()
//      Note:
//   Example: $MRHDG,213,T*2F

// 0 $MRHDG                Sentence type
// 1 [Heading]             Current heading in degrees
// 2 [Reference]           Heading reference; “T”-true, “M”-magnetic, “U”-unspecified
// 3 Checksum
bool M300::handleMsgMRHDG(string msg)
{
  if(!strBegins(msg, "$MRHDG,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 4) 
    return(reportBadMessage(msg, "Wrong field count"));

  string str_hdg = flds[1];
  // TODO: convert magnetic to true if heading is magnetic
  double dbl_hdg = atof(str_hdg.c_str());
  m_nav_hdg = dbl_hdg;

  return(true);
}

//----------------------------------------------------------
// Procedure: handleMsgMRHDG()
//      Note:
//   Example: $MRSPW,1.3,MS*66

// 0 $MRSPW                Sentence type, contains water speed only
// 1 [Water speed]         Speed over water
// 2 [Unit]                “KT”-knots, “MS”-meters/second, “KM”-km/h, “U”-unspecified
// 3 Checksum
bool M300::handleMsgMRSPW(string msg)
{
  if(!strBegins(msg, "$MRSPW,"))
      return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 4) 
    return(reportBadMessage(msg, "Wrong field count"));

  string str_v_unit = flds[2];
  string str_vw  = flds[1];

  double dbl_vw = atof(str_vw.c_str());
  double dbl_mps;
  // Convert water speed to meters per second
  // TODO: turn this into a reusable "convert_mps" functions
  if(str_v_unit == "KT")
    dbl_mps = dbl_vw * 0.514444;
  else if(str_v_unit == "KM")
    dbl_mps = dbl_vw * 0.277778;
  else if(str_v_unit == "MS")
    dbl_mps = dbl_vw;
  else
  {
    // TODO: post warning
    dbl_mps = dbl_vw;
  }
  m_nav_spd = dbl_mps;

  return(true);

}

//----------------------------------------------------------
// Procedure: handleMsgMRGNS()
//      Note:
//   Example: $MRGNS,44.0888889,9.84861111*50

// 0 $MRGNS                Sentence type, contains GNSS data
// 1 [Latitude]            Decimal degrees
// 2 [Longitude]           Decimal degrees
// 3 Checksum
bool M300::handleMsgMRGNS(string msg)
{
  if(!strBegins(msg, "$MRGNS,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 4)
    return(reportBadMessage(msg, "Wrong field count"));

  string str_lat = flds[1];
  string str_lon = flds[2];
  double dbl_lat = atof(str_lat.c_str());
  double dbl_lon = atof(str_lon.c_str());
  
  double x, y;
  bool ok = m_geodesy.LatLong2LocalGrid(dbl_lat, dbl_lon, y, x);
  if(ok) {
    m_nav_x = x;
    m_nav_y = y;
    Notify("NAV_X", x, "GPRMC");
    Notify("NAV_Y", y, "GPRMC");
  }

  return(true);
}


//----------------------------------------------------------
// Procedure: handleMsgMRFSS()
//      Note: Proper NMEA format and checksum prior confirmed  
//   Example: $MRFSS,1560450607,IDLE*40

// 0 $MRFSS                Sentence type, contains frontseat state info
// 1 [Timestamp]           Linux epoch in seconds of state change
// 2 [State]               "PAYLOAD" - backseat is in control, "IDLE" - backseat can ask to start, "MANUAL" - backseat is ignored
// 3 Checksum
bool M300::handleMsgMRFSS(string msg)
{
  if(!strBegins(msg, "$MRFSS,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 4)
    return(reportBadMessage(msg, "Wrong field count"));

  string str_epoch = flds[1];
  double dbl_epoch = atof(str_epoch.c_str());
  string str_new_state = flds[2];

  // Do nothing if state didn't change
  
  // Retract warning if new state is not MANUAL anymore
  if(str_new_state != "MANUAL" && m_fs_state == MANUAL)
  {
    retractRunWarning("Frontseat has taken control: MOOS commands will be ignored");
  }

  // Set state enum based off message content
  if(str_new_state == "MANUAL" && m_fs_state != MANUAL)
  {
    m_fs_state = MANUAL;
    reportRunWarning("Frontseat has taken control: MOOS commands will be ignored");
  }
  else if(str_new_state == "IDLE")
  {
    m_fs_state = IDLE;
  }
  else if(str_new_state == "PAYLOAD")
  {
    m_fs_state = PAYLOAD;
  }
  return(true);
}


// END new MR message handlers
//---------------------------------------------------------


//---------------------------------------------------------
// Procedure: handleMsgGPRMC()
//      Note: Proper NMEA format and checksum prior confirmed  
//   Example:
//   $GPRMC,150942.619,A,0000.00,N,00000.00,W,1.1663,0,291263,0,E*41

//  0   $GPRMC
//  1 [Timestamp]    UTC of position fix
//  2 [Data status]  A-ok, V-invalid
//  3 [Lat_NMEA]     Calculated latitude, in NMEA format
//  4 [LatNS_NMEA]   Hemisphere (N or S) of latitude
//  5 [Lon_NMEA]     Calculated longitude, in NMEA format
//  6 [LonEW_NMEA]   Hemisphere (E or W) of longitude
//  7 [Speed]        Speed over ground in Knots
//  8 [Course]       True Course, Track made good in degrees
//  9 [DepthTop]     Date of Fix
// 10 [Mag Var]      Magnetic variation degrees
// 11 [Mag Var E/W]  Easterly subtracts from true course

bool M300::handleMsgGPRMC(string msg)
{
  if(!strBegins(msg, "$GPRMC,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 13) 
    return(reportBadMessage(msg, "Wrong field count"));
  
  if((flds[4] != "N") && (flds[4] != "S"))
    return(reportBadMessage(msg, "Bad N/S Hemisphere"));
  if((flds[6] != "W") && (flds[4] != "E"))
    return(reportBadMessage(msg, "Bad E/W Hemisphere"));
  
  string str_lat = flds[3];
  string str_lon = flds[5];
  string str_kts = flds[7];
  string str_hdg = flds[8];
  if(!isNumber(str_lat))
    return(reportBadMessage(msg, "Bad Lat"));
  if(!isNumber(str_lon))
    return(reportBadMessage(msg, "Bad Lon"));
  if(!isNumber(str_kts))
    return(reportBadMessage(msg, "Bad Kts"));
  if(!isNumber(str_hdg))
    return(reportBadMessage(msg, "Bad Hdg"));
  
  double dbl_lat = latDDMMtoDD(str_lat);
  double dbl_lon = lonDDDMMtoDDD(str_lon);
  if(flds[4] == "S")
    dbl_lat = -dbl_lat;
  if(flds[6] == "W")
    dbl_lon = -dbl_lon;
  Notify("NAV_LAT", dbl_lat, "GPRMC");
  Notify("NAV_LON", dbl_lon, "GPRMC");

  double x, y;
  bool ok = m_geodesy.LatLong2LocalGrid(dbl_lat, dbl_lon, y, x);
  if(ok) {
    m_nav_x = x;
    m_nav_y = y;
    Notify("NAV_X", x, "GPRMC");
    Notify("NAV_Y", y, "GPRMC");
  }
  
  double dbl_kts = atof(str_kts.c_str());
  double dbl_mps = dbl_kts * 0.514444;
  dbl_mps = snapToStep(dbl_mps, 0.05);
  m_nav_spd = dbl_mps;
  Notify("NAV_SPEED", dbl_mps, "GPRMC");
  
  double dbl_hdg = atof(str_hdg.c_str());
  m_nav_hdg = dbl_hdg;
  if (!use_imu_heading){
    Notify("NAV_HEADING", dbl_hdg, "GPRMC");
  }
  else{
    Notify("GPS_HEADING", dbl_hdg, "GPRMC");
  }
  return(true);
}


//---------------------------------------------------------
// Procedure: handleMsgGPGGA()
//      Note: Proper NMEA format and checksum prior confirmed  
//      Note: Only grabbing the number of satellites from this msg
//   Example:
//   $GPGGA,150502.00,4221.46039,N,07105.28402,W,2,11,0.98,5.5,M,-33.2,M,,0000*62
//                                                 ^^
//  0   $GPGGA
//  1 [Timestamp]    UTC of position fix
//  2 [Lat_NMEA]     Calculated latitude, in NMEA format
//  3 [LatNS_NMEA]   Hemisphere (N or S) of latitude
//  4 [Lon_NMEA]     Calculated longitude, in NMEA format
//  5 [LonEW_NMEA]   Hemisphere (E or W) of longitude
//  6 [GPS Qual]     (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
//  7 [Num Sats]     Num satellites in use, not those in view
//  8 [Horz Dilu]    Horizontal dilution of position
//  9 [Ant Alt]      Antenna altitude above/below mean sea level (geoid)
// 10 [Ant Units]    Meters  (Antenna height unit)
// 11 [Geo Sep]      Geoidal separation (Diff. between WGS-84 earth
//                   ellipsoid and mean sea level.
// 12 [GS Units]     Meters  (Units of geoidal separation)
// 13 [Age]          in secs since last update from diff. ref station
// 14 [Diff ID]     Diff. reference station ID#

bool M300::handleMsgGPGGA(string msg)
{
  if(!strBegins(msg, "$GPGGA,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 14) 
    return(reportBadMessage(msg, "Wrong field count"));
  
  string str_sats = flds[7];
  if(!isNumber(str_sats))
    return(reportBadMessage(msg, "Bad Sats"));
  
  int int_sats = atoi(str_sats.c_str());
  Notify("GPS_SATS", int_sats, "GPGGA");

  if(int_sats < 0)
    int_sats = 0;
  m_num_satellites = (unsigned int)(int_sats);
  
  return(true);
}


//---------------------------------------------------------
// Procedure: handleMsgCPNVG()
//      Note: Proper NMEA format and checksum prior confirmed  
//   Example:
//   $CPNVG,160743.715,0000.00,N,00000.00,W,1,,,0,,,160743.715*64
//      0      1         2     3   4      5 6   9      12
//
//  0   CPNVG
//  1 [Timestamp]    Timestamp of the sentence
//  2 [Lat_NMEA]     Calculated latitude, in NMEA format
//  3 [LatNS_NMEA]   Hemisphere (N or S) of latitude
//  4 [Lon_NMEA]     Calculated longitude, in NMEA format
//  5 [LonEW_NMEA]   Hemisphere (E or W) of longitude
//  6 [PosQual]      Quality of position est (no GPS=0, otherwise=1)
//  7 [AltBottom]    Alt in meters from bottom, blank for USVs
//  8 [DepthTop]     Dep in meters from top, blank for USVs
//  9 [Heading]      Dir of travel in degs clockwise from true north
// 10 [Roll]         Degrees of roll
// 11 [Pitch]        Degrees of pitch
// 12 [NavTimestamp] Timestamp for time this pose/position

bool M300::handleMsgCPNVG(string msg)
{
  if(!strBegins(msg, "$CPNVG,"))
    return(false);
  
  // Remove the checksum info from end
  rbiteString(msg, '*');
  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 13) 
    return(reportBadMessage(msg, "Wrong field count"));
  
  if((flds[3] != "N") && (flds[3] != "S"))
    return(reportBadMessage(msg, "Bad N/S Hemisphere"));
  if((flds[5] != "W") && (flds[5] != "E")) 
    return(reportBadMessage(msg, "Bad E/W Hemisphere"));
  
  string str_lat = flds[2];
  string str_lon = flds[4];
  string str_hdg = flds[9];
  if(!isNumber(str_lat))  
    return(reportBadMessage(msg, "Bad Lat"));
  if(!isNumber(str_lon)) 
    return(reportBadMessage(msg, "Bad Lon"));
  if(!isNumber(str_hdg)) 
    return(reportBadMessage(msg, "Bad Hdg"));
  
  double dbl_lat = latDDMMtoDD(str_lat);
  double dbl_lon = lonDDDMMtoDDD(str_lon);
  if(flds[3] == "S")
    dbl_lat = -dbl_lat;
  if(flds[5] == "W")
    dbl_lon = -dbl_lon;
  Notify("NAV_LAT", dbl_lat, "CPNVG");
  Notify("NAV_LON", dbl_lon, "CPNVG");

  double x, y;
  bool ok = m_geodesy.LatLong2LocalGrid(dbl_lat, dbl_lon, y, x);
  if(ok) {
    m_nav_x = x;
    m_nav_y = y;
    Notify("NAV_X", x, "GPRMC");
    Notify("NAV_Y", y, "GPRMC");
  }
  
  double dbl_hdg = atof(str_hdg.c_str());
  m_nav_hdg = dbl_hdg;
  Notify("NAV_HEADING", dbl_hdg, "CPNVG");
  return(true);
}

//---------------------------------------------------------
// Procedure: handleMsgCPNVG_heading()
//      Note: Proper NMEA format and checksum prior confirmed  
// This function will only publish NAV_HEADING from the 
// $CPNVG message

bool M300::handleMsgCPNVG_heading(string msg)
{
  if(!strBegins(msg, "$CPNVG,"))
    return(false);
  
  // Remove the checksum info from end
  rbiteString(msg, '*');
  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 13) 
    return(reportBadMessage(msg, "Wrong field count"));
  
  // if((flds[3] != "N") && (flds[3] != "S"))
  //   return(reportBadMessage(msg, "Bad N/S Hemisphere"));
  // if((flds[5] != "W") && (flds[5] != "E")) 
  //   return(reportBadMessage(msg, "Bad E/W Hemisphere"));
  
  // string str_lat = flds[2];
  // string str_lon = flds[4];
  string str_hdg = flds[9];
  // if(!isNumber(str_lat))  
  //   return(reportBadMessage(msg, "Bad Lat"));
  // if(!isNumber(str_lon)) 
  //   return(reportBadMessage(msg, "Bad Lon"));
  if(!isNumber(str_hdg)) 
    return(reportBadMessage(msg, "Bad Hdg"));
  
  // double dbl_lat = latDDMMtoDD(str_lat);
  // double dbl_lon = lonDDDMMtoDDD(str_lon);
  // if(flds[3] == "S")
  //   dbl_lat = -dbl_lat;
  // if(flds[5] == "W")
  //   dbl_lon = -dbl_lon;
  // Notify("NAV_LAT", dbl_lat, "CPNVG");
  // Notify("NAV_LON", dbl_lon, "CPNVG");

  // double x, y;
  // bool ok = m_geodesy.LatLong2LocalGrid(dbl_lat, dbl_lon, y, x);
  // if(ok) {
  //   m_nav_x = x;
  //   m_nav_y = y;
  //   Notify("NAV_X", x, "GPRMC");
  //   Notify("NAV_Y", y, "GPRMC");
  // }
  
  double dbl_hdg = atof(str_hdg.c_str());
  m_nav_hdg = dbl_hdg;
  Notify("NAV_HEADING", dbl_hdg, "CPNVG");
  return(true);
}


//---------------------------------------------------------
// Procedure: handleMsgCPRBS()
//      Note: Proper NMEA format and checksum prior confirmed  
//   Example: $CPRBS,091945.064,1,15.2,15.1,15.3,0*57
//               0      1       2   3   4    5   6 HH
//
//  0   CPRBS
//  1 [Timestamp]     Timestamp of the sentence.
//  2 [ID_Battery]    Unique ID of battery being reported on.
//  3 [V_Batt_Stack]  Voltage of the battery bank.
//  4 [V_Batt_Min]    Lowest voltage read from cells in bank.
//  5 [V_Batt_Max]    Highest voltage read from cells in bank. 
//  6 [TemperatureC]  Temperature in Celsius.

bool M300::handleMsgCPRBS(string msg)
{
  if(!strBegins(msg, "$CPRBS,"))
    return(false);
  
  // Remove the checksum info from end
  rbiteString(msg, '*');
  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 6) 
    return(reportBadMessage(msg, "Wrong field count"));
  
  string str_voltage = flds[3];
  if(!isNumber(str_voltage))
    return(reportBadMessage(msg, "Bad Voltage"));
  
  double dbl_voltage = atof(str_voltage.c_str());
  m_batt_voltage = dbl_voltage;
  Notify("M300_BATT_VOLTAGE", dbl_voltage, "CPRBS");
  return(true);
}


//---------------------------------------------------------
// Procedure: reportBadMessage()
  
bool M300::reportBadMessage(string msg, string reason)
{
  reportRunWarning("Bad NMEA Msg: " + reason + ": " + msg);
  Notify("IM300_BAD_NMEA", reason + ": " + msg);
  return(false);
}

//---------------------------------------------------------
// Procedure: reportWarningsEvents()
//      Note: Get the AppCast-consistent events, warnings and
//            retractions from the sock ninja for posting

void M300::reportWarningsEvents()
{
  // Part 1: Handle Event Messages()
  list<string> events = m_ninja.getEvents();
  list<string>::iterator p;  
  for(p=events.begin(); p!=events.end(); p++) {
    string event_str = *p;
    reportEvent(event_str);
  }

  // Part 2: Handle Warning Messages()
  list<string> warnings = m_ninja.getWarnings();
  list<string> thrust_warnings = m_thrust.getWarnings();
  warnings.splice(warnings.end(), thrust_warnings);
  for(p=warnings.begin(); p!=warnings.end(); p++) {
    string warning_str = *p;
    reportRunWarning(warning_str);
  }

  // Part 3: Handle Retraction Messages()
  list<string> retractions = m_ninja.getRetractions();
  for(p=retractions.begin(); p!=retractions.end(); p++) {
    string retraction_str = *p;
    retractRunWarning(retraction_str);
  }
}
  
//------------------------------------------------------------
// Procedure: buildReport()
//
// -------------------------------------------
// Config:   max_r/t: 30/100      stale_check:  false
//           dr_mode: normal      stale_thresh: 15
// -------------------------------------------
// Drive     des_rud: -30         des_thrust_L: 0
// State:    des_thr: 40          des_thrust_R: 0
// -------------------------------------------
// Nav:      nav_x: 5968          nav_hdg: 0
//           nav_y: -6616.3       nav_spd: 0.5
// -------------------------------------------
// System:   voltage: 15.2        satellites: 0
// -------------------------------------------
// Comms:    Type: client         IPv4: 127.0.0.1 (of server)
//           Format: nmea         Port: 29500
//           Status: connected
// ---------------------------
// NMEA sentences:
// <--R     230  $CPNVG,105707.24,0000.00,N,00000.00,W,1,,,0,,,105707.24*64
// <--R     230  $CPRBS,105707.24,1,15.2,15.1,15.3,0*67
// <--R     230  $GPRMC,105707.24,A,0000.00,N,00000.00,W,1.1663,0,291263,0,E*76
//  S-->    231  $PYDIR,0,0*56


 
bool M300::buildReport() 
{
  string str_max_rud  = doubleToStringX(m_max_rudder,1);
  string str_max_thr  = doubleToStringX(m_max_thrust,1);
  string str_max_both = str_max_rud + "/" + str_max_thr;
  string str_des_rud  = doubleToStringX(m_thrust.getRudder(),1);
  string str_des_thr  = doubleToStringX(m_thrust.getThrust(),1);
  string str_des_thrL = doubleToStringX(m_thrust.getThrustLeft(),1);
  string str_des_thrR = doubleToStringX(m_thrust.getThrustRight(),1);
  
  string str_sta_thr  = doubleToStringX(m_stale_threshold,1);
  string str_sta_ena  = boolToString(m_stale_check_enabled);

  string str_nav_x   = doubleToStringX(m_nav_x,1);
  string str_nav_y   = doubleToStringX(m_nav_y,1);
  string str_nav_hdg = doubleToStringX(m_nav_hdg,1);
  string str_nav_spd = doubleToStringX(m_nav_spd,1);
  string str_voltage = doubleToStringX(m_batt_voltage,1);
  string str_sats    = uintToString(m_num_satellites);


  string pd_ruth = padString(str_max_both, 10, false);
  string pd_drmo = padString(m_drive_mode, 10, false);
  string pd_drud = padString(str_des_rud, 10, false);
  string pd_dthr = padString(str_des_thr, 10, false);
  string pd_navx = padString(str_nav_x, 10, false);
  string pd_navy = padString(str_nav_y, 10, false);
  string pd_volt = padString(str_voltage, 10, false);

  
  m_msgs << "Config:    max_r/t: " << pd_ruth << "   stale_check:  " << str_sta_ena << endl;
  m_msgs << "           dr_mode: " << pd_drmo << "   stale_thresh: " << str_sta_thr << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "Drive:     des_rud: " << pd_drud << "   des_thrust_L: " << str_des_thrL << endl;
  m_msgs << "State:     des_thr: " << pd_dthr << "   des_thrust_R: " << str_des_thrR << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "Nav:       nav_x:   " << pd_navx << "   nav_hdg: " << str_nav_hdg << endl;
  m_msgs << "           nav_y:   " << pd_navy << "   nav_spd: " << str_nav_spd << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "System:    voltage: " << pd_volt << "   satellites: " << str_sats << endl;
  m_msgs << "------------------------------------------------------" << endl;
  
  list<string> summary_lines = m_ninja.getSummary();
  list<string>::iterator p;
  for(p=summary_lines.begin(); p!=summary_lines.end(); p++) {
    string line = *p;
    m_msgs << line << endl;
  }

  return(true);
}
