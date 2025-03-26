#!/bin/sh

if [ "$1" = "windows" ]; then
    wine "$2" -no-date -force -o "$3" - < "$4"
else
    "$2" -force -o "$3" - < "$4"
fi
