#!/bin/bash

# make sure the text file includes a UTF-8 byte order mark
# and has Windows style line endings (\r\n), so we can test
# if gendlopen handles it correctly

f="helloworld.txt"
mv -f $f ${f}.old
printf '\xEF\xBB\xBF' > $f
sed -e 's|\xEF\xBB\xBF||g; s|\r||' ${f}.old | sed -e 's|$|\r|' >> $f
