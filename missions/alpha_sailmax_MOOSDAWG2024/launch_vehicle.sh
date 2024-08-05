#!/bin/bash -e
#-------------------------------------------------------------- 
#   Script: launch_vehicle.sh                                    
#  Mission: alpha_sailmax
#   Author: Michael Sacarny  
#     Date: October 2022
#--------------------------------------------------------------
#  Part 1: Set global var defaults
#--------------------------------------------------------------
ME=`basename "$0"`
GRN='\033[0;32m'
NC='\033[0m' # No Color

TIME_WARP=${TIME_WARP:-1}
JUST_MAKE="no"
VERBOSE=${VERBOSE:-"no"}
CONFIRM=${CONFIRM:-"yes"}
AUTO_LAUNCHED=${AUTO_LAUNCHED:-"no"}
CMD_ARGS=""


MOOS_PORT=${MOOS_PORT:-"9001"}
PSHARE_PORT=${PSHARE_PORT:-"9201"}

# these settings are for running IRL
SHORE_IP=${SHORE_IP:-"127.0.0.1"}
SHORE_PSHARE=${SHORE_PSHARE:-"9200"}
VNAME=${VNAME:-"sally"}
INDEX=${INDEX:-"1"}
XMODE="SAILBOAT"
FSEAT_IP="192.168.1.160"
BSEAT_IP="192.168.1.158"
IP_ADDR="192.168.1.158"

REGION=${REGION:-"pavlab"}
START_POS=${START_POS:-"40,-40"}
BHV_MFILE="meta_vehicle.bhv"

# From Ray Turissi Sept 9,2022
DEFAULT_POLAR="0,0: 6,9: 18,7: 30,23: 45,41: 57,37: 69,37: 81,45: 93,53: 105,62: 117,60: 129,49: 141,69: 153,86: 168,79: 180,80"
POLAR=${POLAR:-$DEFAULT_POLAR}
#Note: POLAR STRING DOES NOT MAKE IT THROUGH NSPLUG INTO TARG FILES!!!!

SPEED=${SPEED:-"2.0"}
MAXSPD=${MAXSP:-"2"}
RETURN_POS=${RETURN_POS:-"5,0"}

#--------------------------------------------------------------
#  Part 2: Check for and handle command-line arguments
#--------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [OPTIONS] [time_warp]                        "
	echo "                                                 " 
	echo "Options:                                         "
	echo "  --help, -h                                     " 
	echo "    Print this help message and exit             "
	echo "  --just_make, -j                                " 
	echo "    Just make targ files, but do not launch      "
	echo "  --verbose, -v                                  " 
	echo "    Verbose output, confirm before launching     "
	echo "  --noconfirm, -nc                               " 
	echo "    No confirmation before launching             "
  echo "  --auto, -a                                     "
  echo "     Auto-launched by a script.                  "
  echo "     Will not launch uMAC as the final step.     "
	echo "                                                 "
	echo "  --ip=<localhost>                               " 
	echo "    Force pHostInfo to use this IP Address       "
	echo "  --mport=<9001>                                 "
	echo "    Port number of this vehicle's MOOSDB port    "
	echo "  --pshare=<9201>                                " 
	echo "    Port number of this vehicle's pShare port    "
	echo "                                                 "
	echo "  --shore=<192.168.1.237>                        " 
	echo "    IP address location of shoreside             "
	echo "  --vname=<abe>                                  " 
	echo "    Name of the vehicle being launched           " 
	echo "  --index=<1>                                    " 
	echo "    Index for setting MOOSDB and pShare ports    "
	echo "                                                 "
	echo "  --start=<X,Y>     (default is 0,0)             " 
	echo "    Start position chosen by script launching    "
	echo "    this script (to ensure separation)           "
	echo "  --speed=meters/sec                             " 
	echo "    The speed use for transiting/loitering       "
	echo "  --maxspd=meters/sec                            " 
	echo "    Max speed of vehicle (for sim and in-field)  "
	echo "                                                 "
	echo "  --deer            Set region to be Deer Island "	
	echo "  --sim, -s    : Use uSimMarine                  "
	echo "  --simd, -d   : Use Docker simulation           "
	echo "  --mission-pt2pt, -mp                           " 
	echo "    Use the Point-to-Point mision                "
  echo "  --shape=[tri:rect:oct]                         "
  echo "    Selects test shape                           "
  echo "  --order=[normal:reverse]                       "
  echo "    Order to run pointsse                        "
  echo "  --repeat=<n>                                   "
  echo "    Number of repeats after initial run          "
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

    elif [ "${ARGI:0:8}" = "--shore=" ]; then
        SHORE_IP="${ARGI#--shore=*}"
    elif [ "${ARGI:0:8}" = "--vname=" ]; then
        VNAME="${ARGI#--vname=*}"
    elif [ "${ARGI:0:8}" = "--index=" ]; then
        INDEX="${ARGI#--index=*}"
	
    elif [ "${ARGI:0:8}" = "--start=" ]; then
        START_POS="${ARGI#--start=*}"
    elif [ "${ARGI:0:8}" = "--speed=" ]; then
        SPEED="${ARGI#--speed=*}"
    elif [ "${ARGI:0:9}" = "--maxspd=" ]; then
        MAXSPD="${ARGI#--maxspd=*}"
    elif [ "${ARGI}" = "--pavlab" -o "${ARGI}" = "-p" ]; then
        REGION="pavlab"
    elif [ "${ARGI}" = "--forest" -o "${ARGI}" = "-f" ]; then
        REGION="forest_lake"
    elif [ "${ARGI}" = "--deer" ]; then
        REGION="deer_island"        
    elif [ "${ARGI}" = "--mission-pt2pt" -o "${ARGI}" = "-mp" ]; then
        BHV_MFILE="meta_vehicle_pp.bhv"
    elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
        XMODE="SIM"
        echo "Simulation mode ON."
    elif [ "${ARGI}" = "--simd" -o "${ARGI}" = "-d" ]; then
        XMODE="SIMD"
        echo "Docker simulation mode ON."   
    elif [ "${ARGI:0:8}" = "--shape=" ]; then
        SHAPE="${ARGI#--shape=*}"
    elif [ "${ARGI:0:8}" = "--order=" ]; then
        ORDER="${ARGI#--order=*}" 
    elif [ "${ARGI:0:9}" = "--repeat=" ]; then
        REPEAT="${ARGI#--repeat=*}"
    elif [ "${ARGI:0:9}" = "--wspeed=" ]; then
        :    
    elif [ "${ARGI:0:7}" = "--wdir=" ]; then
        :          
    elif [ "${ARGI:0:8}" = "--wfile=" ]; then
        :  
    else
	echo "$ME: Bad Arg:[$ARGI]. Exit Code 1."
	exit 1
    fi
done

MOOS_PORT=`expr $INDEX + 9000`
PSHARE_PORT=`expr $INDEX + 9200`

# sim settings
if [ "${XMODE}" = "SIM" ]; then
    IP_ADDR="localhost"
    SHORE_IP="localhost"
elif [ "${XMODE}" = "SIMD" ]; then
    IP_ADDR="localhost"
    SHORE_IP="localhost"
    FSEAT_IP="0.0.0.0"
fi
     
#---------------------------------------------------------------
#  Part 3: If verbose, show vars and confirm before launching
#---------------------------------------------------------------
if [ "${VERBOSE}" = "yes" -o "${CONFIRM}" = "yes" ]; then 
    echo "$ME"
    echo "CMD_ARGS =      [${CMD_ARGS}]     "
    echo "TIME_WARP =     [${TIME_WARP}]    "
    echo "AUTO_LAUNCHED = [${AUTO_LAUNCHED}]"
    echo "CONFIRM =       [${CONFIRM}]      "
    echo "----------------------------------"
    echo "MOOS_PORT =     [${MOOS_PORT}]    "
    echo "PSHARE_PORT =   [${PSHARE_PORT}]  "
    echo "IP_ADDR =       [${IP_ADDR}]      "
    echo "----------------------------------"
    echo "SHORE_IP =      [${SHORE_IP}]     "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}] "
    echo "VNAME =         [${VNAME}]        "
    echo "INDEX =         [${INDEX}]        "
    echo "----------------------------------"
    echo "FSEAT_IP =      [${FSEAT_IP}]     "
    echo "XMODE =         [${XMODE}]        "
    echo "----------------------------------"
    echo "START_POS =     [${START_POS}]    "
    echo "SPEED =         [${SPEED}]        "
    echo "MAXSPD =        [${MAXSPD}]       "
    echo "REGION =        [${REGION}]       "
    echo "SHAPE =         [${SHAPE}]        "
    echo "ORDER =         [${ORDER}]        "
    echo "REPEAT =        [${REPEAT}]       "    
fi

if [ "${CONFIRM}" = "yes" ]; then 
    echo -n "Hit any key to continue with launching"
    read ANSWER
fi


#--------------------------------------------------------------
#  Part 4: Create the .moos and .bhv files. 
#--------------------------------------------------------------
NSFLAGS="-s -f"
if [ "${AUTO}" = "" ]; then
    NSFLAGS="-i -f"
fi

nsplug meta_vehicle.moos targ_$VNAME.moos $NSFLAGS WARP=$TIME_WARP \
       PSHARE_PORT=$PSHARE_PORT     VNAME=$VNAME                   \
       START_POS=$START_POS         SHORE_IP=$SHORE_IP             \
       SHORE_PSHARE=$SHORE_PSHARE   MOOS_PORT=$MOOS_PORT           \
       IP_ADDR=$IP_ADDR             REGION=$REGION                 \
       FSEAT_IP=$FSEAT_IP           XMODE=$XMODE                   \
       START_POS=$START_POS         MAXSPD=$MAXSPD                 \
       POLAR=$POLAR                 XMODE=$XMODE

nsplug meta_vehicle.bhv targ_$VNAME.bhv $NSFLAGS VNAME=$VNAME \
       START_POS=$START_POS         REGION=$REGION            \
       SPEED=$SPEED                 POLAR=$POLAR              \
       SHAPE=$SHAPE                 ORDER=$ORDER              \
       REPEAT=$REPEAT
       
if [ ${JUST_MAKE} = "yes" ] ; then
    echo "Files assembled; nothing launched; exiting per request."
    exit 0
fi

#--------------------------------------------------------------
#  Part 5: Launch the processes
#--------------------------------------------------------------
echo -e "$GRN Launching $VNAME MOOS Community. WARP=$TIME_WARP $NC"
pAntler targ_$VNAME.moos >& /dev/null &
echo -e "$GRN Done Launching the $VNAME MOOS Community $NC"

#---------------------------------------------------------------
#  Part 6: If launched from script, we're done, exit now
#---------------------------------------------------------------
if [ "${AUTO_LAUNCHED}" = "yes" ]; then
    exit 0
fi

#---------------------------------------------------------------
# Part 7: Launch uMAC until the mission is quit
#---------------------------------------------------------------
uMAC targ_$VNAME.moos
kill -- -$$
