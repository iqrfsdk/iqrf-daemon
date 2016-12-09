#!/bin/bash
#
# Daemon Name: iqrf_startup_bg
#  
# chkconfig: - 58 74
# description: A script starting IQRF startup backgroun daemon
# ls daemons: ps -eo 'tty,pid,comm' | grep ^?
# http://shahmirj.com/blog/the-initd-script
# http://www.debian.org/doc/debian-policy/ch-opersys.html#s-sysvinit

# Source function library.
. /etc/init.d/functions

# Source networking configuration.
. /etc/sysconfig/myconfig

prog=iqrf_startup_bg
lockfile=/var/lock/subsys/$prog

start() { 
    #Make some checks for requirements before continuing
    [ "$NETWORKING" = "no" ] && exit 1
    [ -x /usr/sbin/$prog ] || exit 5

    # Start our daemon daemon
    echo -n $"Starting $prog: "
    daemon --pidfile /var/run/${proc}.pid $prog
    RETVAL=$?
    echo

    #If all is well touch the lock file
    [ $RETVAL -eq 0 ] && touch $lockfile
    return $RETVAL
}

stop() {
    echo -n $"Shutting down $prog: "
    killproc $prog
    RETVAL=$?
    echo

    #If all is well remove the lockfile
    [ $RETVAL -eq 0 ] && rm -f $lockfile
    return $RETVAL
}

# See how we were called.
case "$1" in
  start)
        start
        ;;
  stop)
        stop
        ;;
  status)
        status $prog
        ;;
  restart)
        stop
        start
        ;;
   *)
        echo $"Usage: $0 {start|stop|status|restart}"
        exit 2
esac
