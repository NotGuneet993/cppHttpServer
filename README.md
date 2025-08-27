# cppHttpServer
Making a HTTP server in C++ from scratch 

# Documentation used
https://bhch.github.io/posts/2017/11/writing-an-http-server-from-scratch/
https://docs.python.org/3/howto/sockets.html
https://developer.mozilla.org/en-US/docs/Web/HTTP

## Step 1 
Create a simple echo TCP server. I'll start by making an echo server.

Go through the whole socket lifecycle: socket -> bind -> listen -> accept -> recv/send -> close.