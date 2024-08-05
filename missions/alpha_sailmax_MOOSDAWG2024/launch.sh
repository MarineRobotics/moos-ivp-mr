#!/bin/bash -e
#--------------------------------------------------------------
#   Script: launch_shoreside.sh                                    
#   Author: Michael Benjamin  
#     Date: June 2022
#--------------------------------------------------------------  
#  Part 1: Set Exit actions and declare global var defaults
#--------------------------------------------------------------
ME=`basename "$0"`
TIME_WARP=5
PASS_ARGS=""
JUST_MAKE="no"

#--------------------------------------------------------------  
#  Part 2: Check for and handle command-line arguments
#--------------------------------------------------------------  
for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [SWITCHES] [time_warp]         "
	echo "  --help, -h                                     " 
	echo "    Display this help message                    "
	echo "  --just_make, -j                                " 
	echo "    Just make targ files, but do not launch      "
	echo "  --pavlab, -p                                   "
	echo "    Set region to be MIT pavlab                  "
  echo "  --deer                                         "
  echo "    Set region to be Deer Island                 "	
  echo "  --sim, -s                                      "
  echo "    Use uSimMarine                               "
  echo "  --simd, -d                                     "
  echo "    Use docker simulator                         "
  echo "  --shape=[tri:rect:oct]                         "
  echo "    Selects test shape                           "
  echo "  --order=[normal:reverse]                       "
  echo "    Order to run points                          "
  echo "  --repeat=<n>                                   "
  echo "    Number of repeats after initial run          "
  echo "  --wspeed=<n>                                   "
  echo "    Wind speed in m/s                            "
  echo "  --wdir=<n>                                     "
  echo "    Wind direction in True degrees               "
  echo "  --wfile=wind_file                              "
  echo "    Path from $HOME to wind data file            "  
  
	exit 0;
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
	PASS_ARGS+=" --just_make"
	JUST_MAKE="yes"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI}" = "--deer" ]; then
        PASS_ARGS+=" $ARGI"        
    elif [ "${ARGI}" = "--noconfirm" -o "${ARGI}" = "-nc" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI}" = "--simd" -o "${ARGI}" = "-d" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI}" = "-f" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI:0:8}" = "--shape=" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI:0:8}" = "--order=" ]; then
        PASS_ARGS+=" $ARGI"
    elif [ "${ARGI:0:9}" = "--repeat=" ]; then
        PASS_ARGS+=" $ARGI"
   elif [ "${ARGI:0:9}" = "--wspeed=" ]; then
        PASS_ARGS+=" $ARGI"
   elif [ "${ARGI:0:7}" = "--wdir=" ]; then
        PASS_ARGS+=" $ARGI"  
   elif [ "${ARGI:0:8}" = "--wfile=" ]; then
        PASS_ARGS+=" $ARGI"                       
    else 
        echo "launch.sh Bad arg:" $ARGI " Exiting with code: 1"
        exit 1
    fi
done


#--------------------------------------------------------------  
#  Part 3: Actual launch
#--------------------------------------------------------------  
./launch_vehicle.sh   --auto $PASS_ARGS $TIME_WARP --vname=sally
sleep 1
./launch_shoreside.sh --auto $PASS_ARGS $TIME_WARP

#---------------------------------------------------------------
# Part 4: Launch uMAC until the mission is quit
#---------------------------------------------------------------
uMAC targ_shoreside.moos
kill -- -$$

