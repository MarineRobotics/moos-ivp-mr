/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300.cpp                                        */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

/************************************************************/
/*    EDITED BY: Vincent Vandyck                            */
/*    ORGN: Marine Robotics                                 */
/*    FILE: SailBoat.cpp                                    */
/*    DATE: 1 Nov 2021                                      */
/************************************************************/

#include "MBUtils.h"
#include "LatLonFormatUtils.h"
#include "SailBoat.h"

using namespace std;

//---------------------------------------------------------
// Constructor

SailBoat::SailBoat()
{
  // Configuration variables  (overwritten by .moos params)
  //m_drive_mode   = "normal";    // default DRIVE_MODE ("normal"|"aggro")

  // TODO: implement max speed if necessary
  m_max_speed      = 2.0;

  m_ivp_allstop      = true;

  // Stale Message Detection
  m_stale_check_enabled = false;
  m_stale_mode          = false;
  m_stale_threshold     = 1.5;
  m_count_stale         = 0;
  m_tstamp_des_heading  = 0;
  m_tstamp_des_speed    = 0;

  m_bad_nmea_semantic   = 0;

  m_nav_x   = -1;
  m_nav_y   = -1;
  m_nav_hdg = -1;
  m_nav_spd = -1;

    m_fs_state = UNDEFINED;
}

//---------------------------------------------------------
// Destructor

SailBoat::~SailBoat()
{
  m_ninja.closeSockFDs();
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool SailBoat::OnStartUp()
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
    else if(param == "max_speed")
      handled = setPosDoubleOnString(m_max_speed, value);
    // TODO: should we use something similar to drive mode
    //       to set the sailing mode, therefore set in mission file
    //else if(param == "drive_mode")
    //  handled = m_thrust.setDriveMode(value);
    else if(param == "ignore_msg") 
      handled = handleConfigIgnoreMsg(value);

    if(!handled){
      reportUnhandledConfigWarning(orig);
    }
  }
  
  // Init Geodesy 
  GeodesySetup();
  
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool SailBoat::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void SailBoat::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("IVPHELM_ALLSTOP", 0);
  Register("DESIRED_HEADING", 0);
  Register("DESIRED_SPEED", 0);
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool SailBoat::OnNewMail(MOOSMSG_LIST &NewMail)
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
    // TODO: Add a sail mode to this for future use
    // This probbly means: 
    // - sail mode (powerboat - hybrid - purist)

    if(key == "IVPHELM_ALLSTOP")
      m_ivp_allstop = (toupper(sval) != "CLEAR");
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

// TODO: continue here
bool SailBoat::Iterate()
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

bool SailBoat::GeodesySetup()
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

void SailBoat::sendMessagesToSocket()
{
  // Send primary MRCMD front seat command
  string msg = "MRCMD,";
  msg += doubleToStringX(m_curr_time,1) + ",";
  msg += doubleToStringX(m_des_heading,1) + ",";
  msg += "T,";;
  msg += doubleToStringX(m_des_speed,1) + ",";
  msg += m_des_speed_unit;
  msg = "$" + msg + "*" + checksumHexStr(msg) + "\r\n";

  m_ninja.sendSockMessage(msg);
  
  // Publish command to MOOSDB for logging/debugging
  // TODO: notify cmd command sent
  }

//---------------------------------------------------------
// Procedure: readMessagesFromSocket()
//      Note: Messages returned from the SockNinja have been
//            confirmed to be valid NMEA format and checksum

void SailBoat::readMessagesFromSocket()
{
  list<string> incoming_msgs = m_ninja.getSockMessages();
  list<string>::iterator p;
  for(p=incoming_msgs.begin(); p!=incoming_msgs.end(); p++) {
    string msg = *p;
    msg = biteString(msg, '\r'); // Remove CRLF
    Notify("ISailBoat_RAW_NMEA", msg);

    bool handled = false;
    if(m_ignore_msgs.count(msg.substr (0,6)) >= 1) 
      handled = true;

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
    else if (strBegins(msg, "$MRMWV"))
      handled = handleMsgMRMWV(msg);
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

bool SailBoat::handleConfigIgnoreMsg(string str)
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
// Procedure: checkForStalenessOrAllStop()
//   Purpose: If DESIRED_HEADING or DESIRED_SPEED commands are stale,
//            set local desired_speed to zero.
//            If an all-stop has been posted, also set the
//            local desired_speed val to zero.

void SailBoat::checkForStalenessOrAllStop()
{
  if(m_ivp_allstop) {
    m_des_speed = 0;
    return;
  }

  // If not checking staleness, ensure stale mode false, return.
  // TODO: stale check is disabled by default
  if(!m_stale_check_enabled) {
    m_stale_mode = false;
    return;
  }
  double lag_heading = m_curr_time - m_tstamp_des_heading;
  double lag_speed = m_curr_time - m_tstamp_des_speed;

  bool stale_heading = (lag_heading > m_stale_threshold);
  bool stale_speed = (lag_speed > m_stale_threshold);

  
  if(stale_heading)
    m_count_stale++;
  if(stale_speed)
    m_count_stale++;

  bool stale_mode = false;
  if(stale_heading || stale_speed) {
    // TODO: it would be cool to make the boat
    //       weatherhelm here instead of just setting speed to 0
    //       same goes for all stop
    m_des_speed = 0;
    stale_mode = true;
  }

  // Check new stale_mode represents a change from previ ous
  if(stale_mode && !m_stale_mode) 
    reportRunWarning("Stale Command Detected: Stopping Vehicle");
  if(!stale_mode && m_stale_mode) 
    retractRunWarning("Stale Command Detected: Stopping Vehicle");

  m_stale_mode = stale_mode;
}

//---------------------------------------------------------
// MR message handlers

//---------------------------------------------------------
// Procedure: checkFrontSeatState()
//   Purpose: Handle the current frontseat state appropriately. If:
//            - "MANUAL": publish all_stop.
//            - "IDLE" and not stale: send start command to front seat
//            - "PAYLOAD": Continue

void SailBoat::checkFrontSeatState()
{
  switch(m_fs_state)
  {
    case MANUAL:
      if(!m_ivp_allstop)
      {
        Notify("IVPHELM_ALLSTOP", "Frontseat has taken control");
      }
      break;
    //TODO: Should we notify manual override to get rid of this allstop?
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

bool SailBoat::handleMsgMRINF(string msg)
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
  
  cout << "LAT: " << str_lat << "LON: " << str_lon << endl;

  double dbl_lat = atof(str_lat.c_str());
  double dbl_lon = atof(str_lon.c_str());

  Notify("NAV_LAT", dbl_lat, "MRINF");
  Notify("NAV_LON", dbl_lon, "MRINF");
  
  double x, y;
  bool ok = m_geodesy.LatLong2LocalGrid(dbl_lat, dbl_lon, y, x);
  if(ok) {
    m_nav_x = x;
    m_nav_y = y;
    Notify("NAV_X", x, "MRINF");
    Notify("NAV_Y", y, "MRINF");
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
  Notify("NAV_SPEED", dbl_mps, "MRINF");
  // TODO: convert magnetic to true if heading is magnetic
  double dbl_hdg = atof(str_hdg.c_str());
  m_nav_hdg = dbl_hdg;
  Notify("NAV_HEADING", dbl_hdg, "MRINF");

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
bool SailBoat::handleMsgMRHDG(string msg)
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
bool SailBoat::handleMsgMRSPW(string msg)
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
bool SailBoat::handleMsgMRGNS(string msg)
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
// 2 [State]               "PAYLOAD" - backseat is in control, "IDLE" - backseat can ask to start, "FRONTSEAT_MISSION" - backseat is ignored
bool SailBoat::handleMsgMRFSS(string msg)
{
  if(!strBegins(msg, "$MRFSS,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 3)
    return(reportBadMessage(msg, "Wrong field count"));

  string str_epoch = flds[1];
  double dbl_epoch = atof(str_epoch.c_str());
  string str_new_state = flds[2];

  // Do nothing if state didn't change
  
  // Retract warning if new state is not MANUAL anymore
  if(str_new_state != "FRONTSEAT_MISSION" && m_fs_state == MANUAL)
  {
    reportEvent("Frontseat has taken control: MOOS commands will be ignored");
  }

  // Set state enum based off message content
  if(str_new_state == "FRONTSEAT_MISSION" && m_fs_state != MANUAL)
  {
    m_fs_state = MANUAL;
    reportEvent("Frontseat has taken control: MOOS commands will be ignored");
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

//----------------------------------------------------------
// Procedure: handleMsgMRMWV()
//      Note: Proper NMEA format and checksum prior confirmed
//   Example: $MRMWV,270,T,8,N,A*29

// 0 $MRMWV                Sentence type, contains frontseat state info
// 1 [Wind angle]          0.0 to 359.9 degrees, in relation to the vessel’s bow/centerline, to the nearest 0.1
//                         degree. If the data for this field is not valid, the field will be blank.
// 2 [Reference]           R = Relative (apparent wind, as felt when standing on the moving ship)
//                         T = Theoretical (calculated actual wind, as though the vessel were stationary)
// 3 [Wind speed]          Wind speed, to the nearest tenth of a unit.  If the data for this field is not valid, the field will be blank.
// 4 [Wind speed units]    K = km/hr
//                         M = m/s
//                         N = knots
//                         S = statute miles/hr
// 5 [Status]              A = data valid; V = data invalid
bool SailBoat::handleMsgMRMWV(string msg)
{
  if(!strBegins(msg, "$MRMWV,"))
    return(false);

  // Remove the checksum info from end
  rbiteString(msg, '*');

  vector<string> flds = parseString(msg, ',');
  if(flds.size() != 6)
    return(reportBadMessage(msg, "Wrong field count"));

  string str_angle = flds[1];
  double dbl_angle = atof(str_angle.c_str());
  string str_reference = flds[2];
  string str_speed = flds[3];
  double dbl_speed = atof(str_speed.c_str());
  string str_unit = flds[4];
  string str_status = flds[5];

  // TODO: check if true or apparent, publish accordingly

  // Convert speed to M/S if necessary
  if(str_unit == "K")
    dbl_speed = dbl_speed * 2.77778;
  else if(str_unit == "N")
    dbl_speed = dbl_speed * 0,514444;
  else if(str_unit == "S")
    dbl_speed = dbl_speed * 0,44704;

  if(str_reference == "R")
  {
    // publish relative wind
    m_wind_speed_r = dbl_speed;
    m_wind_angle_r = dbl_angle;
    Notify("WIND_SPEED_R", dbl_speed, "MRMWV");
    Notify("WIND_ANGLE_R", dbl_angle, "MRMWV");

  }
  else
  {
    m_wind_speed_t = dbl_speed;
    m_wind_angle_t = dbl_angle;
    Notify("WIND_SPEED_T", dbl_speed, "MRMWV");
    Notify("WIND_ANGLE_T", dbl_angle, "MRMWV");
  }

  return(true);
}

// END new MR message handlers
//---------------------------------------------------------


//---------------------------------------------------------
// Procedure: reportBadMessage()
  
bool SailBoat::reportBadMessage(string msg, string reason)
{
  reportRunWarning("Bad NMEA Msg: " + reason + ": " + msg);
  Notify("ISailBoat_BAD_NMEA", reason + ": " + msg);
  return(false);
}

//---------------------------------------------------------
// Procedure: reportWarningsEvents()
//      Note: Get the AppCast-consistent events, warnings and
//            retractions from the sock ninja for posting

void SailBoat::reportWarningsEvents()
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
  //list<string> thrust_warnings = m_thrust.getWarnings();
  //warnings.splice(warnings.end(), thrust_warnings);
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


 
bool SailBoat::buildReport() 
{
  //string str_max_rud  = doubleToStringX(m_max_rudder,1);//
  //string str_max_thr  = doubleToStringX(m_max_thrust,1);//
  //string str_max_both = str_max_rud + "/" + str_max_thr;//
  
  // max speed
  // desired speed
  // desired heading

  string str_sta_thr  = doubleToStringX(m_stale_threshold,1);
  string str_sta_ena  = boolToString(m_stale_check_enabled);

  string str_nav_x   = doubleToStringX(m_nav_x,1);
  string str_nav_y   = doubleToStringX(m_nav_y,1);
  string str_nav_hdg = doubleToStringX(m_nav_hdg,1);
  string str_nav_spd = doubleToStringX(m_nav_spd,1);
    
  string str_state   = uintToString(m_fs_state);


  //string pd_ruth = padString(str_max_both, 10, false);//
  //string pd_drmo = padString(m_drive_mode, 10, false);//
  //string pd_drud = padString(str_des_rud, 10, false);//
  //string pd_dthr = padString(str_des_thr, 10, false);//
  string pd_navx = padString(str_nav_x, 10, false);
  string pd_navy = padString(str_nav_y, 10, false);
  //string pd_volt = padString(str_voltage, 10, false);//

  string pd_stat = padString(str_state, 10, false);

  
  m_msgs << "Config:    max_r/t: " << "pd_ruth" << "   stale_check:  " << str_sta_ena << endl;
  m_msgs << "           fs_state:" << pd_stat << "   stale_thresh: " << str_sta_thr << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "Drive:     des_rud: " << "pd_drud" << "   des_thrust_L: " << "str_des_thrL" << endl;
  m_msgs << "State:     des_thr: " << "pd_dthr" << "   des_thrust_R: " << "str_des_thrR" << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "Nav:       nav_x:   " << pd_navx << "   nav_hdg: " << str_nav_hdg << endl;
  m_msgs << "           nav_y:   " << pd_navy << "   nav_spd: " << str_nav_spd << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "System:    voltage: " << "pd_volt" << "   satellites: " << "str_sats" << endl;
  m_msgs << "------------------------------------------------------" << endl;
  m_msgs << "Longitude: " << m_longitude << endl;
  
  list<string> summary_lines = m_ninja.getSummary();
  list<string>::iterator p;
  for(p=summary_lines.begin(); p!=summary_lines.end(); p++) {
    string line = *p;
    m_msgs << line << endl;
  }

  return(true);
}
