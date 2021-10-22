# Linux_file_server
Linux file server and client written in C

• Transfer files from the directory: /root/server/ to the client directory: /root/client/
• File transfer will support text and binary files
• File transfer will occur over port 1337
• The client will connect to the server at 127.0.0.1, obtain a list of files in the /root/server directory and
present a menu to the user with a list of files and the option of downloading one of the files or exiting.
• File lengths will be checked before and after file transfer.
• The client will also be written in C and run on a Kali Linux OS, but from the /root/client directory.
• Where practical use POSIX-compliant networking functions.
• Two separate programs (server.c and client.c) are required but they may be combined into one program
if the user specifies --server (for server) or --client (for client) at the command line when launching the
program.
• The server must only communicate with the client and must not prompt for input once launched.
• The server may display status and diagnostic information on the STDERR device.
