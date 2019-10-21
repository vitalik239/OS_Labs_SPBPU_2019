ps -ajx | grep "./daemon" | grep -v "grep"
cat /var/log/syslog | tail -30 | grep " daemon"
