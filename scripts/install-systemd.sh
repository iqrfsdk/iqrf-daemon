#!/usr/bin/env bash
# Script for installing IQRF daemon service for systemd
#
# Service interface for later control
# sudo systemctl start/stop/status/... IQRF
# or
# sudo service IQRF start/stop/status

set -e

case $# in
  0)
    name="IQRF"
    command="/usr/local/bin/iqrf_startup"
    workdir="/usr/local/bin"
    args="/usr/local/bin/configuration/config.json"
    ;;
  2)
    name=$1
    command=$(readlink -f $2)
    workdir=$(dirname $command)
    args=" "
    ;;
  3)
    name=$1
    command=$(readlink -f $2)
    workdir=$(dirname $command)
    args=$3
    ;;
  *)
    echo "Unknown number of parameters: $#"
    echo "Usage: ./install-systemd.sh install default IQRF service for iqrf_startup"
    echo "       ./install-systemd.sh <NAME> <COMMAND> install user specifed service named <NAME>"
    echo "                                             for command <COMMAND>"
    echo "       ./install-systemd.sh <NAME> <COMMAND> <ARGS> install user specifed service named <NAME>"
    echo "                                                    for command <COMMAND> with command arguments <ARGS>"
    exit 1
    ;;
esac

cat > /lib/systemd/system/"$name".service << END
[Unit]
Description=$name daemon
After=syslog.target network.target

[Service]
Type=simple
User=root
Group=root
WorkingDirectory=$workdir
ExecStart=$command $args
StandardOutput=syslog
StandardError=syslog
RestartSec=5
Restart=always

[Install]
WantedBy=multi-user.target

END


chmod 664 /lib/systemd/system/"$name".service

systemctl enable "$name".service
systemctl --system daemon-reload
systemctl start "$name".service
