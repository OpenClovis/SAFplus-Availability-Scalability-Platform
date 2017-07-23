#!/bin/sh
index=0
until [ $index -gt 3 ]
do
   echo "looping $index ***********************"
   ./alarm_client
   index=`expr $index + 1`
   sleep 20
done
echo "final index $index"
echo "good bye all!"
