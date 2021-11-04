/************************************************************/
/*    NAME: Blake Cole                                      */
/*    ORGN: MIT                                             */
/*    FILE: main.cpp                                        */
/*    DATE: 01 APRIL 2020                                   */
/************************************************************/

/************************************************************/
/*    EDITED BY: Vincent Vandyck                            */
/*    ORGN: Marine Robotics                                 */
/*    FILE: main.cpp                                        */
/*    DATE: 1 Nov 2021                                      */
/************************************************************/


#include <string>
#include "MBUtils.h"
#include "ColorParse.h"
#include "SailBoat.h"
#include "SailBoat_Info.h"

using namespace std;

int main(int argc, char *argv[])
{
  string mission_file;
  string run_command = argv[0];

  for(int i=1; i<argc; i++) {
    string argi = argv[i];
    if((argi=="-v") || (argi=="--version") || (argi=="-version"))
      showReleaseInfoAndExit();
    else if((argi=="-e") || (argi=="--example") || (argi=="-example"))
      showExampleConfigAndExit();
    else if((argi == "-h") || (argi == "--help") || (argi=="-help"))
      showHelpAndExit();
    else if((argi == "-i") || (argi == "--interface"))
      showInterfaceAndExit();
    else if(strEnds(argi, ".moos") || strEnds(argi, ".moos++"))
      mission_file = argv[i];
    else if(strBegins(argi, "--alias="))
      run_command = argi.substr(8);
    else if(i==2)
      run_command = argi;
  }
  
  if(mission_file == "")
    showHelpAndExit();

  cout << termColor("green");
  cout << "iSailBoat launching as " << run_command << endl;
  cout << termColor() << endl;

  SailBoat SailBoat;

  SailBoat.Run(run_command.c_str(), mission_file.c_str());
  
  return(0);
}

