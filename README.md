# Scalable Server

## Usage

    Usage: ./server.out -m [select|epoll] -p [port] -b [buffer size]
        -m - The operatin mode. Either 'select' or 'epoll.
        -p - The port to listen on. Must be greater than 1024.
        -b - The buffer size. Recommendation is less than 1000.