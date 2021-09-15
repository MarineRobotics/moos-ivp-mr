##############################
#   HOW TO RUN AFTER BUILD   #
##############################
#
# To be able to use the graphical UI features of this image,
# make sure to run it with the following command, which should enable the
# Docker container to use the system's display invironment
# docker run -ti --net=host --ipc=host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v $XAUTHORITY:/tmp/.XAuthority -e XAUTHORITY=/tmp/.XAuthority --env="QT_X11_NO_MITSHM=1" <name>:<version>



# Import the Marine Robotics frontseat docker container
FROM mrobotics/mr-frontseat-sim:1.1

# Change user to root and install all necessary dependencies
USER root

RUN apt-get update && apt-get install -y \
    subversion \
    netbase \
    g++ cmake xterm \
    libfltk1.3-dev  freeglut3-dev  libpng-dev  libjpeg-dev \
    libxft-dev  libxinerama-dev   libtiff5-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy over the marine robotics moos-ivp tree
WORKDIR $HOME
COPY . ./moos-ivp-mr
# Change ownership of copied folder to mruser (copy sets ownership to root by default)
RUN chown -R $USER:mrgroup ./moos-ivp-mr

# Change user make to parent image user and change to user home directory
# Note: these environmental variables are defined in the parent image
USER $USER
RUN echo $USER
WORKDIR $HOME

# Clone most recent moos-ivp distro
RUN svn co https://oceanai.mit.edu/svn/moos-ivp-aro/trunk/ moos-ivp
WORKDIR $HOME/moos-ivp


# Build MOOS-IvP
RUN ./build-moos.sh
RUN ./build-ivp.sh

ENV PATH "$PATH:$HOME/moos-ivp/bin"

# Build the Marine Robotics moos-ivp tree
WORKDIR $HOME/moos-ivp-mr
RUN ./build.sh

ENV PATH "$PATH:$HOME/moos-ivp-mr/bin"
USER $USER