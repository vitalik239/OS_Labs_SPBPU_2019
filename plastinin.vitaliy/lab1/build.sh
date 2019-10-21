g++ daemon.cpp -Wall -Werror -o daemon
rm -f daemon.o

pid_file="/var/run/daemon.pid"
echo '' > "$pid_file"
chmod 666 "$pid_file"