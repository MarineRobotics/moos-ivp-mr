#!/bin/bash -e
#--------------------------------------------------------------
#   Script: launch_shoreside.sh                                    
#  Mission: alpha_sailmax
#   Author: Michael Sacarny  
#   LastEd: October 2022     
#--------------------------------------------------------------  
#  Part 1: Set global variables
#--------------------------------------------------------------
ME=`basename "$0"`
GRN='\033[0;32m'
NC='\033[0m' # No Color

TIME_WARP=1
JUST_MAKE="no"
VERBOSE=""
CONFIRM="yes"
AUTO_LAUNCHED="no"
CMD_ARGS=""

IP_ADDR="localhost"
MOOS_PORT="9000"
PSHARE_PORT="9200"

REGION=${REGION:-"pavlab"}
SHAPE=${SHAPE:-"sh_triangle"}
POLAR=${POLAR:-""}
XMODE="SAILBOAT"

#--------------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#--------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME: [OPTIONS] [time_warp]                       " 
	echo "                                                 "
	echo "Options:                                         "
    echo "  --help, -h                                     "
    echo "    Display this help message                    "
	echo "  --just_make, -j                                " 
	echo "    Just make target files. Do not launch.       "
    echo "  --verbose, -v                                  "
    echo "    Increase verbosity                           "
	echo "  --noconfirm, -nc                               " 
	echo "    No confirmation before launching             "
    echo "  --auto, -a                                     "
    echo "    Auto-launched by a script.                   "
    echo "    Will not launch uMAC as the final step.      "
	echo "                                                 "
	echo "  --ip=<localhost>                               " 
	echo "    Force pHostInfo to use this IP Address       "
	echo "  --mport=<9000>                                 "
	echo "    Port number of this vehicle's MOOSDB port    "
	echo "  --pshare=<9200>                                " 
	echo "    Port number of this vehicle's pShare port    "
	echo "                                                 "
	echo "  --pavlab, -p      Set region to be MIT pavlab  "
  echo "  --forest, -f      Set region to be Forest Lake "
  echo "  --deer            Set region to be Deer Island "
	echo "  --sim, -s         Use uSimMarine               "
	echo "  --simd, -d        Use Docker simulator         "
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
	JUST_MAKE="yes"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE="yes"
    elif [ "${ARGI}" = "--noconfirm" -o "${ARGI}" = "-nc" ]; then
	CONFIRM="no"
    elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
        AUTO_LAUNCHED="yes"
    elif [ "${ARGI:0:5}" = "--ip=" ]; then
        IP_ADDR="${ARGI#--ip=*}"
    elif [ "${ARGI:0:7}" = "--mport" ]; then
	MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        REGION="pavlab"
    elif [ "${ARGI}" = "--forest" -o "${ARGI}" = "-f" ]; then
        REGION="forest_lake"
    elif [ "${ARGI}" = "--deer" ]; then
        REGION="deer_island"         
    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        XMODE="SIM"
        echo "Simulation mode ON."
    elif [ "${ARGI}" = "--simd" -o "${ARGI}" = "-d" ]; then
        XMODE="SIMD"
        echo "Docker simulation mode ON."
    elif [ "${ARGI:0:9}" = "--wspeed=" ]; then
        WSPEED="${ARGI#--wspeed=*}"    
    elif [ "${ARGI:0:7}" = "--wdir=" ]; then
        WDIR="${ARGI#--wdir=*}"          
    elif [ "${ARGI:0:8}" = "--wfile=" ]; then
        WFILE="${ARGI#--wfile=*}"  
    elif [ "${ARGI:0:8}" = "--shape=" ]; then
        :
    elif [ "${ARGI:0:8}" = "--order=" ]; then
        :
    elif [ "${ARGI:0:9}" = "--repeat=" ]; then
        :
    else 
	echo "$ME: Bad Arg: $ARGI. Exit Code 1."
	exit 1
    fi
done

#---------------------------------------------------------------
#  Part 3: If verbose, show vars and confirm before launching
#---------------------------------------------------------------
if [ "${VERBOSE}" = "yes" -o "${CONFIRM}" = "yes" ]; then 
    echo "$ME"
    echo "CMD_ARGS =      [${CMD_ARGS}]      "
    echo "TIME_WARP =     [${TIME_WARP}]     "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}] "
    echo "IP_ADDR =       [${IP_ADDR}]       "
    echo "MOOS_PORT =     [${MOOS_PORT}]     "
    echo "PSHARE_PORT =   [${PSHARE_PORT}]   "
    echo "XMODE =         [${XMODE}]         "
    echo "---------------------------------- "
    echo "REGION =        [${REGION}]        "
    echo "WIND SPEED =    [${WSPEED}]        "
    echo "WIND DIR =      [${WDIR}]          "
    echo "WIND FILE =     [${WFILE}]         "          
    echo -n "Hit any key to continue with launching"
    read ANSWER
fi

#--------------------------------------------------------------
#  Part 4: Create the .moos and .bhv files using nsplug
#--------------------------------------------------------------
nsplug meta_shoreside.moos targ_shoreside.moos -i -f WARP=$TIME_WARP  \
       IP_ADDR=$IP_ADDR       PSHARE_PORT=$PSHARE_PORT                \
       MOOS_PORT=$MOOS_PORT   REGION=$REGION                          \
       XMODE=$XMODE           WSPEED=$WSPEED                          \
       WDIR=$WDIR             WFILE=$WFILE


if [ ${JUST_MAKE} = "yes" ]; then
    echo "Files assembled; nothing launched; exiting per request."
    exit 0
fi

#--------------------------------------------------------------
#  Part 5: Launch the processes
#--------------------------------------------------------------
echo -e "$GRN Launching Shoreside MOOS Community. WARP=$TIME_WARP $NC"
pAntler targ_shoreside.moos >& /dev/null &
echo -e "$GRN Done Launching Shoreside Community $NC"

#---------------------------------------------------------------
#  Part 6: If launched from script, we're done, exit now
#---------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

#---------------------------------------------------------------
# Part 7: Launch uMAC until the mission is quit
#---------------------------------------------------------------
uMAC targ_shoreside.moos
kill -- -$$
