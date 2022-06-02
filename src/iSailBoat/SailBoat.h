/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: M300.h                                          */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

/************************************************************/
/*    EDITED BY: Vincent Vandyck                            */
/*    ORGN: Marine Robotics                                 */
/*    FILE: SailBoat.h                                      */
/*    DATE: 1 Nov 2021                                      */
/************************************************************/

#ifndef SailBoat_HEADER
#define SailBoat_HEADER

#include <string>
#include "SockNinja.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class SailBoat : public AppCastingMOOSApp
{
public:
  SailBoat();
  ~SailBoat();

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

  // Original iSailBoat message handlers
  bool handleConfigIgnoreMsg(std::string);

  // New Marine Robotics message handlers
  bool handleMsgMRINF(std::string);
  bool handleMsgMRHDG(std::string);
  bool handleMsgMRSPW(std::string);
  bool handleMsgMRGNS(std::string);
  bool handleMsgMRFSS(std::string);
  bool handleMsgMRMWV(std::string);

  bool reportBadMessage(std::string msg, std::string reason="");
  bool GeodesySetup();
  void checkForStalenessOrAllStop();

  // New Marine Robotics
  void checkFrontSeatState();

private: // Config variables
  double       m_max_speed;        // MAX_SPEED m/s
  std::string  m_drive_mode;       // DRIVE_MODE


  std::set<std::string> m_ignore_msgs;
  
private: // State variables
  CMOOSGeodesy m_geodesy;
  SockNinja    m_ninja;

  //TODO: remove this from the cpp file
  //Thruster     m_thrust;

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
  
  // MR Stale Message Detection
  double       m_tstamp_des_heading;
  double       m_tstamp_des_speed;
  
  double       m_nav_x;
  double       m_nav_y;
  double       m_nav_hdg;
  double       m_nav_spd;

  double       m_wind_speed_r;
  double       m_wind_angle_r;
  double       m_wind_speed_t;
  double       m_wind_angle_t;

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
