#!/bin/bash

build() {
	local DIR="$1"
	cd "$DIR" && cd ..
    make
}

startbg() {
	local PROG="$1"
	./"$PROG" -m epoll -p 8000 -b 100 &
}

main() {
    # Rebuild the server if needed
	local DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null 2>&1 && pwd)"
    build "$DIR"
    # Start the epoll server in the background
    cd "$DIR" && cd ..
    startbg server.out
}



main "$@"
