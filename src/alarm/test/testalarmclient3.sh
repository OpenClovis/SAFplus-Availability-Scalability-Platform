#!/bin/sh
index=0
until [ $index -gt 10 ]
do
   echo "looping $index ***********************"
   ./alarm_client1&
   ./alarm_client2&
   ./alarm_client3
   index=`expr $index + 1`
   sleep 20
done
echo "final index $index"
echo "good bye all!"
