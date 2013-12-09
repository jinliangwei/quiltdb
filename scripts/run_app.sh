#!/bin/bash

if [ $# -ne 3 ]; then
  echo "usage: $0 <app_prog> <machine file> <config-file>"
  exit
fi


script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
third_party_dir=${project_root}/third_party_installed/lib/

app_prog=`readlink -f $1`
config_file=`readlink -f $3`

machine_file=$2
ip_list=`cat $machine_file`

id=0
for ip in $ip_list; do

    id2=$(( id+1 ))

    cmd="LD_LIBRARY_PATH=$third_party_dir:${LD_LIBRARY_PATH} \
    GLOG_logtostderr=true \
    GLOG_v=2, 
    GLOG_vmodule=receiver=2,config_parser=-1,timer_thr=0 \
    GLOG_minloglevel=0 \
    $app_prog --config_file ${config_file} 
    --myhid ${id} --myvid ${id2}"
    
    echo "Running app on $ip with"
    echo
    echo $cmd
    echo "=============================="
    echo 

    ssh -oStrictHostKeyChecking=no $ip $cmd &
    id=$(( id+2 ))
done