#! /bin/sh
# file guis_demo.sh
# $Id: guis_demo.sh 1.1 Sat, 30 Aug 2003 14:33:51 +0200 basile $

echo "you may run this $0 script with -T option for protocol tracing" 
echo ".. and -D for debug message"

echo "you need to have . in your PATH or" 
echo "... to put guisdemo_script guisdemo_client pyguis pyguis-scripter into your PATH"

guisdemo_script -p guisdemo_client $*
#eof $Id: guis_demo.sh 1.1 Sat, 30 Aug 2003 14:33:51 +0200 basile $