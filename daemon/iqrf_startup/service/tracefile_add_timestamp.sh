#!/bin/bash
#Log file renaming

DAEMON_LOG_FILE=/var/log/iqrf-daemon.log
TIMESTAMP=$(date "+%Y.%m.%d-%H.%M.%S")

if [ -f "${DAEMON_LOG_FILE}" ]; then
	echo "IQRF daemon log file renaming ..."
	mv ${DAEMON_LOG_FILE} ${DAEMON_LOG_FILE}.${TIMESTAMP}
else
	echo "IQRF daemon log file does not exist!"
fi
