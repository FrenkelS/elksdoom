export ELKSTOPDIR=/home/frenkel/elks
export WATCOM=/usr/bin/watcom

export INCLUDE=$ELKSTOPDIR/libc/include:$ELKSTOPDIR/include:$ELKSTOPDIR/elks/include:$WATCOM/h:$INCLUDE
export LIBC=$ELKSTOPDIR/libc/libcm.lib
export PATH=$WATCOM/binl64:$WATCOM/binl:$PATH
