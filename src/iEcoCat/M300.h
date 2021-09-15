/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300.h                                          */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

#ifndef M300_HEADER
#define M300_HEADER

#include <string>
#include "SockNinja.h"
#include "Thruster.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class M300 : public AppCastingMOOSApp
{
public:
  M300();
  ~M300();

protected: // Standard public MOOSApp functions
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

protected: // Standard protected MOOS App functions
  bool buildReport();
  void registerVariables();

protected: // App Specific functions
  void reportWarningsEvents();
  void sendMessagesToSocket();
  void readMessagesFromSocket();

  // Original iM300 message handlers
  bool handleConfigIgnoreMsg(std::string);
  bool handleMsgGPRMC(std::string);
  bool handleMsgGPGGA(std::string);
  bool handleMsgCPNVG(std::string);
  bool handleMsgCPNVG_heading(std::string);
  bool handleMsgCPRBS(std::string);
  bool handleConfigUseImuHdg(std::string);

  // New Marine Robotics message handlers
  bool handleMsgMRINF(std::string);
  bool handleMsgMRHDG(std::string);
  bool handleMsgMRSPW(std::string);
  bool handleMsgMRGNS(std::string);
  bool handleMsgMRFSS(std::string);

  bool reportBadMessage(std::string msg, std::string reason="");
  bool GeodesySetup();
  void checkForStalenessOrAllStop();

  // New Marine Robotics
  void checkFrontSeatState();

  bool diffThrust(double des_rudder, double des_thrust,
		  double &des_thrustL, double &des_thrustR);

private: // Config variables
  double       m_max_rudder;       // MAX_RUDDER
  double       m_max_thrust;       // MAX_THRUST
  std::string  m_drive_mode;       // DRIVE_MODE
  bool         use_imu_heading;


  std::set<std::string> m_ignore_msgs;
  
private: // State variables
  CMOOSGeodesy m_geodesy;
  SockNinja    m_ninja;
  Thruster     m_thrust;

  //Marine Robotics
  double       m_des_heading;
  double       m_des_speed;
  std::string  m_des_speed_unit;

  double      m_ivp_allstop;

  // Stale Message Detection
  bool         m_stale_check_enabled;
  bool         m_stale_mode;
  double       m_stale_threshold;
  unsigned int m_count_stale;
  double       m_tstamp_des_rudder;
  double       m_tstamp_des_thrust;

  // MR Stale Message Detection
  double       m_tstamp_des_heading;
  double       m_tstamp_des_speed;

  unsigned int m_num_satellites;
  double       m_batt_voltage;
  double       m_nav_x;
  double       m_nav_y;
  double       m_nav_hdg;
  double       m_nav_spd;

  //new Marine Robotics variable
  enum states
    {
      UNDEFINED,
      MANUAL,
      IDLE,
      PAYLOAD
    };
  states       m_fs_state;
  unsigned int m_bad_nmea_semantic;
};

#endif 
