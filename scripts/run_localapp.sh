#!/bin/bash

if [ $# -ne 2 ]; then
  echo "usage: $0 <app_prog> <config-file>"
  exit
fi


script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
third_party_dir=${project_root}/third_party_installed/lib/

app_prog=`readlink -f $1`
config_file=`readlink -f $2`

LD_LIBRARY_PATH=$third_party_dir:${LD_LIBRARY_PATH} \
    GLOG_logtostderr=true \
    GLOG_v=2 \
    GLOG_minloglevel=0 \
    $app_prog --config_file ${config_file}
