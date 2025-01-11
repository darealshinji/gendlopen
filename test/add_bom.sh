#!/bin/bash

f="helloworld.txt"
mv -f $f ${f}.old
printf '\xEF\xBB\xBF' > $f
sed -e 's|\xEF\xBB\xBF||g' ${f}.old >> $f
