#!/bin/bash

if [ $# -ne 1 ]; then
  echo "usage: $0 <app_prog>"
  exit
fi


script_path=`readlink -f $0`
script_dir=`dirname $script_path`
project_root=`dirname $script_dir`
third_party_dir=${project_root}/third_party_installed/lib/

app_prog=`readlink -f $1`

LD_LIBRARY_PATH=$third_party_dir:${LD_LIBRARY_PATH} \
    GLOG_logtostderr=true \
    GLOG_v=2 \
    GLOG_minloglevel=0 \
    $app_prog
