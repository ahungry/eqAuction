USAGE:

Update eqAuction.ini to match your EverQuest Log directory location.

Simply uncomment one of the lines that matches your directory, or add your directory.

The (#) sign at the beginning of a line denotes a comment.

If you would like to compile your own source, feel free.  
The following was used to compile with libcurl (curl.haxx.se) support.

Compiling was done with mingw32 in the past
mingw32-gcc -L curl/lib -I curl/include -llibcurl eqAuction.c -o eqAuction.exe -DCURL_STATICLIB -lws2_32 -lwinmm

Compiling done with cygwin more recently:
gcc -mno-cygwin -L curl/lib -I curl/include -llibcurl eqAuction.c -o eqAuction.exe -DCURL_STATICLIB -lws2_32 -lwinmm

Email: matt@ahungry.com with any errors or issues