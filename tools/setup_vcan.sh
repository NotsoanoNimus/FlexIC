#!/bin/sh

if [ $# -ne 1 -o "$1" == "-h" -o "$1" == "--help" -o "$1" == "?" -o "$1" == "-?" ]; then
  echo "USAGE: $0 {if-name}"
  echo "  Creates a Virtual CAN interface with the given name."
  echo
  exit 1
fi


function err_msg()
{
  echo "ERROR: ${1}."
  echo
  exit ${2:-1}
}

[ `id -u` -ne 0 ] && err_msg "This script must be run as 'root'"

modprobe vcan || err_msg "Could not load the 'vcan' kernel module"
ip link add dev ${1} type vcan || err_msg "Failed to create vcan interface '${1}'"
ip link set up ${1} || err_msg "Failed to bring up vcan interface '${1}'"

echo "==============================================="
ip link show ${1}

echo
echo "> ok - interface '${1}' is now available and up"
exit 0
