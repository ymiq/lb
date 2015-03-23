#!/bin/sh

stop() {
	killall slb
#	killall clb-fcgi
	killall clb-srv
	killall hub
	killall robot
}

start() {
	./bin/robot -p 11000 -d
	usleep 100000
	./bin/hub -p 10000 -d
	usleep 100000
	./bin/slb -p 12000 -d
	usleep 100000
	./bin/clb-srv -p 7000 -d
	usleep 100000
#	fcgistarter -c /home/work/test/lb/bin/clb-fcgi -p 9000 -N 1
#	usleep 100000
}

# Handle how we were called.
case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	*)
		stop
		start
esac
