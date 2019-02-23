#!/bin/bash

subnet=192.168.0
repo=echo-client
exeName=echoc.out
program=./"$repo"/"$exeName"
host=$(hostname -i | cut -d " " -f 2) # gets the IPv4 address of the host
port=8000
str="asdf"
bufLen=100
sendAllArgs="$program $host $port $str $bufLen"

sendall() {
    local sendArgs=("$@")
    printf -v cmdStr '%q ' "${sendArgs[@]}"
	for i in {1..10}
	do
		ssh root@"$subnet"."$i" "bash -c $cmdStr" &
	done
}

sendNMessages() {
	local delay=$(($1/10))
	for i in {1..$1}
	do
		sendall "$sendAllArgs $1 $delay &"
	done
}

myping() {
	for i in {1..10}
	do
		ping -c 1 "$subnet"."$i"
	done
}

case $1 in
	clone)
		sendall "git clone --recurse-submodules https://github.com/krixium/$repo";;
	pull)
		sendall "cd $repo && git add -A && git stash && git pull";;
	clean)
		sendall "cd $repo && make clean";;
	wipe)
		sendall "rm -rf $repo";;
	make)
		sendall "cd $repo && make";;
	single)
		sendall "$sendAllArgs &";;
	thou)
		sendNMessages 100;;
	10thou)
		sendNMessages 1000;;
	100thou)
		sendNMessages 10000;;
	rst)
		sendall "shutdown -r now";;
	kill)
		sendall "pkill $exeName";;
	ping)
		myping;;
	connect)
		ssh root@"$subnet"."$2";;
	*)
		echo "Invalid option";;
esac
