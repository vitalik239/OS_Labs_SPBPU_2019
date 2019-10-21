#!/bin/bash

folder1=`pwd`/folder1
folder2=`pwd`/folder2
folder3=`pwd`/folder3
interval=60
config=conf.txt

echo "$folder1" > "$config"
echo "$folder2" >> "$config"
echo "$interval" >> "$config"

function finish_test {
	rm -rf "$folder1" "$folder2" "$folder3"
	rm -f conf.txt
	sleep 2
	./daemon stop
	exit 0
}

rm -rf "$folder1" "$folder2" 

mkdir "$folder1"
mkdir "$folder1"/fold1
mkdir "$folder1"/fold2
dd if=/dev/zero of="$folder1"/fl1.jpg bs=1M count=3 > /dev/null 2>&1
dd if=/dev/zero of="$folder1"/fl2.png bs=1M count=2 > /dev/null 2>&1
dd if=/dev/zero of="$folder1"/fl3.txt bs=1M count=1 > /dev/null 2>&1

mkdir "$folder2"
dd if=/dev/zero of="$folder2"/fl.txt bs=1M count=1 > /dev/null 2>&1

#test1
before=`ps -ajx | grep " ./daemon" | grep -v "grep" | wc -l`
./daemon start
sleep 2
./daemon start
sleep 2 
./daemon start
sleep 2
after=`ps -ajx | grep " ./daemon" | grep -v "grep" | wc -l`
if [[ $(expr $after - $before) -eq 1 ]]
then
	echo "Test 1. OK"
else 
	printf "Test 1. Fail\n$before\n$after\n"
	finish_test
fi

#test2
sleep 50

difpng=`diff <(find $folder1 -name "*.png" -type f -printf "%f\n" | sort) <(ls -1 $folder2/IMG | sort)`
difothers=`diff <(find $folder1 ! -name "*.png" -type f -printf "%f\n" | sort) <(ls -1 $folder2/OTHERS | sort)`
if [ -z "$difimg" ] && [ -z "$difothers" ] 
then 
	echo "Test 2. OK"
else
	printf "Test 2. Fail\n$difpng\n$difothers"
	finish_test
fi

#test3
rm -rf folder2/*

sleep 50
difpng=`diff <(find $folder1 -name "*.png" -type f -printf "%f\n" | sort) <(ls -1 $folder2/IMG | sort)`
difothers=`diff <(find $folder1 ! -name "*.png" -type f -printf "%f\n" | sort) <(ls -1 $folder2/OTHERS | sort)`
if [ -z "$difimg" ] && [ -z "$difothers" ] 
then 
	echo "Test 3. OK"
else
	printf "Test 3. Fail\n$difpng\n$difothers\n"
	finish_test
fi

#test4
before=`ps -ajx | grep " ./daemon" | wc -l`
./daemon stop
after=`ps -ajx | grep " ./daemon" | wc -l`
if [[ $(expr $after - $before) -eq -1 ]]
then
	echo "Test 4. OK"
else 
	echo "Test 4. Fail"
	finish_test
fi

#test5
before=`ps -ajx | grep " ./daemon" | wc -l`
./daemon stop
sleep 2
./daemon stop
after=`ps -ajx | grep " ./daemon" | wc -l`
if [[ $(expr $after - $before) -eq 0 ]]
then
	echo "Test 5. OK"
else 
	echo "Test 5. Fail"
	finish_test
fi

#test6
./daemon start
sleep 10

echo "$folder1" > "$config"
echo "$folder3" >> "$config"
echo "$interval" >> "$config"
kill -s SIGHUP `cat /var/run/daemon.pid`

sleep 30
difpng=`diff <(find $folder1 -name "*.png" -type f -printf "%f\n" | sort) <(ls -1 $folder3/IMG | sort)`
difothers=`diff <(find $folder1 ! -name "*.png" -type f -printf "%f\n" | sort) <(ls -1 $folder3/OTHERS | sort)`
if [ -z "$difimg" ] && [ -z "$difothers" ] 
then 
	echo "Test 6. OK"
else
	printf "Test 6. Fail\n$difpng\n$difothers\n"
	finish_test
fi

finish_test