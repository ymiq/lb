#!/bin/sh

stop() {
	killall slb
	killall clb-fcgi
	killall clb-srv
	killall hub
	killall robot
}

start() {
	./bin/robot -g 0 -d
	./bin/robot -g 1 -d
	usleep 100000
	./bin/hub -g 0 -d
	./bin/hub -g 1 -d
	usleep 100000
	./bin/slb -g 0 -d
	./bin/slb -g 1 -d
	usleep 100000
	./bin/clb-srv -g 0 -d
	./bin/clb-srv -g 1 -d
	usleep 100000
	
	if [ -e /home/work/test/lb/bin/clb-fcgi ]; then
		fcgistarter -c /home/work/test/lb/bin/clb-fcgi -p 9000 -N 1
	else 
		if [ -e /home/work/lb/bin/clb-fcgi ]; then
			fcgistarter -c /home/work/lb/bin/clb-fcgi -p 9000 -N 1
		fi
	fi
	usleep 100000
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
