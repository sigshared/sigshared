#!/bin/bash

ARCH=$1
SIGSHARED_PATH="/mydata/sigshared/"

if [ -z "$ARCH" ] ; then
  echo "Usage: $0 < sigshared >"
  exit 1
fi

if [ -z "$TMUX" ]; then
  if [ -n "`tmux ls | grep sigshared`" ]; then
    tmux kill-session -t sigshared 
  fi
  tmux new-session -s sigshared -n demo "./set_tmux_master.sh $ARCH $SIGSHARED_PATH"
fi
