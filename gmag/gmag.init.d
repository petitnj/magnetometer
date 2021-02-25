#! /bin/sh
# Author: Tom Keffer <keffer@gmail.com>
# Startup script for Debian derivatives
#
#
### BEGIN INIT INFO
# Provides:          GMAG program
# Required-Start:    $local_fs $remote_fs $syslog $time
# Required-Stop:     $local_fs $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: GMAG program
# Description:       Falcon Magnetometer 
### END INIT INFO

# Do NOT "set -e"

# PATH should only include /usr/* if it runs after the mountnfs.sh script
PATH=/sbin:/usr/sbin:/bin:/usr/bin
GMAG_BIN=/home/pi/code/gmag/GBOMag_Startup.sh
GMAG_CFG=
GMAG_USER=root:root 
DESC="GMAG magnetometer system"
NAME=GBOMag_Test1
PIDFILE=/var/run/$NAME.pid

# Read configuration variable file if it is present
#[ -r /etc/default/$NAME ] && . /etc/default/$NAME

# Exit if the package is not installed
#[ -x "$GMAG_BIN" ] || exit 0

DAEMON=$GMAG_BIN
DAEMON_ARGS="--daemon --pidfile=$PIDFILE $GMAG_CFG" 

# Load the VERBOSE setting and other rcS variables
. /lib/init/vars.sh

# Define LSB log_* functions.
# Depend on lsb-base (>= 3.0-6) to ensure that this file is present.
. /lib/lsb/init-functions

# start the daemon/service
#   0 if daemon has been started
#   1 if daemon was already running
#   2 if daemon could not be started
# check using ps not the pid file.  pid file could be leftover.
do_start() {
    NPROC=$(count_procs)
    if [ $NPROC != 0 ]; then
	return 1
    fi
    start-stop-daemon --start --chuid $GMAG_USER --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_ARGS || return 2
    return 0
}

# stop the daemon/service
#   0 if daemon has been stopped
#   1 if daemon was already stopped
#   2 if daemon could not be stopped
#   other if a failure occurred
do_stop() {
    # bail out if the app is not running
    NPROC=$(count_procs)
    if [ $NPROC = 0 ]; then
        return 1
    fi
    # bail out if there is no pid file
    if [ ! -f $PIDFILE ]; then
        return 1
    fi
    start-stop-daemon --stop --pidfile $PIDFILE
    # we cannot trust the return value from start-stop-daemon
    RETVAL=2
    c=0
    while [ $c -lt 24 -a "$RETVAL" = "2" ]; do
        c=`expr $c + 1`
        # process may not really have completed, so check it
        NPROC=$(count_procs)
        if [ $NPROC = 0 ]; then
	    RETVAL=0
        else
            echo -n "."
            sleep 5
        fi
    done
    if [ "$RETVAL" = "0" -o "$RETVAL" = "1" ]; then
        # delete the pid file just in case
        rm -f $PIDFILE
    fi
    return "$RETVAL"
}

# send a SIGHUP to the daemon/service
do_reload() {
    start-stop-daemon --stop --signal 1 --quiet --pidfile $PIDFILE
    return 0
}

count_procs() {
    NPROC=`ps ax | grep $GMAG_BIN | grep $NAME.pid | wc -l`
    echo $NPROC
}

RETVAL=0
case "$1" in
    start)
	log_daemon_msg "Starting $DESC" "$NAME"
	do_start
	case "$?" in
	    0) log_end_msg 0; RETVAL=0 ;;
	    1) log_action_cont_msg " already running" && log_end_msg 0; RETVAL=0 ;;
	    2) log_end_msg 1; RETVAL=1 ;;
	esac
	;;
    stop)
	log_daemon_msg "Stopping $DESC" "$NAME"
	do_stop
	case "$?" in
	    0) log_end_msg 0; RETVAL=0 ;;
	    1) log_action_cont_msg " not running" && log_end_msg 0; RETVAL=0 ;;
	    2) log_end_msg 1; RETVAL=1 ;;
	esac
	;;
    status)
        NPROC=$(count_procs)
        if [ $NPROC -gt 1 ]; then
            MSG="running multiple times"
	elif [ $NPROC = 1 ]; then
	    MSG="running"
	else
	    MSG="not running"
	fi
	log_daemon_msg "Status of $DESC" "$MSG"
	log_end_msg 0
	RETVAL=0
	;;
    reload|force-reload)
	log_daemon_msg "Reloading $DESC" "$NAME"
	do_reload
	RETVAL=$?
	log_end_msg $RETVAL
	;;
    restart)
	log_daemon_msg "Restarting $DESC" "$NAME"
	do_stop
	case "$?" in
	    0|1)
		do_start
		case "$?" in
		    0) log_end_msg 0; RETVAL=0 ;;
		    1) log_end_msg 1; RETVAL=1 ;; # Old process still running
		    *) log_end_msg 1; RETVAL=1 ;; # Failed to start
		esac
		;;
	    *)
	  	# Failed to stop
		log_end_msg 1
		RETVAL=1
		;;
	esac
	;;
    *)
	echo "Usage: $0 {start|stop|status|restart|reload}"
	exit 3
	;;
esac

exit $RETVAL

