# paroduscl - a client library to interface with the parodus daemon

Requires the parodus daemon running on the target device.

----

## Library usage

Paroduscl must be integrated directly into an application.  Include the file paroduscl.h and link the application with -lparoduscl.

After successfully calling pcl_init, two file descriptors are returned that allow the application to call select to know when data is available to be read from and written to the daemon.  When data is available to be read, the application must call pcl_read which will process incoming data and call appropriate message handlers registered during init.

