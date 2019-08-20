This folder contains the standard MOOS s1_alpha mission, with multiple variations to make testing your setup easier.

The functions of the multiple .moos files are the following:

1. alpha.moos
--------------
This is the default moos ivp alpha.moos file without modifications


2. alpha_abc_robocat.moos
---------------------------
This file is modified to make the alpha mission work with the
abc_frontseat_simulator and abc_frontseat example
provided by the goby underwater project


3. alpha_robocat.moos
---------------------
This file runs the alpha mission using the robocat's frontseat driver
(mr_frontseat_driver). It includes pShare so that pMarineViewer can be run on
an external computer since the RPi can not handle this.
To simulate the boat, run the ros frontseat but use the start_fs_sim launch file.


4. alpha_shore.moos
---------------------
This file should be ran on the computer that will run pMarineViewer
All this file does is run pMarineViewer and pShare. Compare the ip's in
this file and the alpha_robocat file with the ip of your computer and RPi
if this does not work.
