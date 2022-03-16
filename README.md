# MOOS-IvP MR

The MOOS-IvP tree extension for the Marine Robotics Sailbot

## Installing

 * Clone this repo next to your normal `moos-ivp/` directory
 * Run `./build.sh` and wait for the compilation to complete
 * Add the newly generated `bin/` directory to your $PATH environmental variable

## What's Included 
 * iSailBoat: A frontseat interface using SockNinja to communicate with the Sailboat frontseat over TCP

## Using the frontseat sim docker container
This IvP tree can be used together with our docker sim ([download here](https://hub.docker.com/repository/docker/mrobotics/mr-frontseat-sim))
### Launch alpha mission with sim
 **Using ubuntu with default docker settings**
 * `cd missions/sailbot_alpha`
 * `./launch_vehicle.sh --pysim`

 **Using macOS with default docker settings**
 *  `cd missions/sailbot_alpha`
 * `./launch_vehicle.sh --fseat_ip=0.0.0.0 --ip=localhost --shore=localhost`
 
 **Note: if your docker ip is not the default 172.17.0.1 you have 2 options:**  
 (to find your docker ip, run ifconfig, look for the "docker0" network interface)  
 * Option 1: modify launch_vehicle.sh. Change the default "FSEAT_IP" value on line 22 to your docker ip
 * Option 2: use launch arguments: `./launch_vehicle.sh --fseat_ip=<docker ip> --ip=localhost --shore=localhost`


## iSailBoat
iSailBoat has 2 configuration variables that can be changed:
 * `port=54321` default is 54321, this can be changed if the frontseat is found on a different port (for example when the frontseat is running in a docker container)
 * `ip_addr=172.17.0.1` the ip address at which the frontseat can be found
