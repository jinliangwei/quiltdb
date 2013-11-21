#!/bin/bash
# This script will make boost on any given number of cores
# Building with the parallel boost graph you need to include
# else the folwarning: Graph library does not contain MPI-based parallel components.
# note: to enable them, add "using mpi ;" to your user-config.jam

prefix=$1

#echo "You are about to install Boost, do you wish to continue?   [y/n]"
#read a
#if [[ $a == "N" || $a == "n" ]]; then
#        echo "Exiting script so you can read it first."
#else
    cd boost_1_54_0
    # Get the required libraries, main ones are icu for boost::regex support
    #echo "Getting required libraries..."
    #sudo apt-get update
    # simple search to get and grab the files required
    #sudo apt-get install build-essential g++ python-dev autotools-dev libicu-dev build-essential libbz2-dev 

    # boost's bootsrap setup
    ./bootstrap.sh --prefix=$prefix
    
    # If we want MPI then we need to set the flag in the user-config.jam file
    user_configFile=`find $PWD -name user-config.jam`
    #echo "using mpi ;" >> $user_configFile
    
    # Find the maximum number of physical cores
    n=`cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}'`

    # Install boost in parallel
    ./b2 --with=all -j $n install 
    
    # Reset the ldconfig, assumes you have /usr/local/lib setup already. check ROOT installation 
    # blog I wrote if not. Else you can add it to your LD_LIBRARY_PATH, running this anyway
    # will not hurt.
    #sudo ldconfig  
    echo "Boost installation complete."
#fi

