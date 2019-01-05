#!/bin/bash

cd $(dirname "$0")

kill_server() {
	i=0
	while true; do
		response=$(./mcrcon -H "::1" -P "25579" -p "passwordhere" list 2>/dev/null)
		if echo "$response" | grep 'There are 0/'; then
			let i++
		else
			let i=0
		fi

		if [ "$i" -eq 10 ]; then
			#kill minecraft
			./mcrcon -H "::1" -P "25579" -p "passwordhere" stop
		fi

		sleep 60
	done
}

(kill_server) &

while true; do
	#launch minecraft
	cd mc
	java -Xmx3G -Xms512M -jar forge-1.7.10-10.13.4.1558-1.7.10-universal.jar
	cd ..

	#wait for connection
	while ! ./mc_idle_wrapper; do
		echo "mc_idle_wrapper failed. Retrying!"
	done
done
