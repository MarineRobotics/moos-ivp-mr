# MOOS-IvP MR

The MOOS-IvP tree extension for the Marine Robotics Sailbot

## Installing

 * Clone this repo next to your normal `moos-ivp/` directory
 * Run `./build.sh` and wait for the compilation to complete
 * Add the newly generated `bin/` directory to your $PATH environmental variable

## What's Included 
 * iSailBoat: A frontseat interface using SockNinja to communicate with the Sailboat frontseat over TCP

## iSailBoat
iSailBoat has 2 configuration variables that can be changed:
 * `port=54321` default is 54321, this can be changed if the frontseat is found on a different port (for example when the frontseat is running in a docker container)
 * `ip_addr=172.17.0.1` the ip address at which the frontseat can be found
