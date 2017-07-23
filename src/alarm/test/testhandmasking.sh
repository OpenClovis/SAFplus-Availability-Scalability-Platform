#!/bin/sh
index=0
until [ $index -gt 20 ]
do
   echo "looping $index ***********************"
   ./alarm_client_parent&
   ./alarm_client_child
   index=`expr $index + 1`
   sleep 2
done
echo "final index $index"
echo "good bye all!"
