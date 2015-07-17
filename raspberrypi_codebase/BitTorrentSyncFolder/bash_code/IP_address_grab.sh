#!/bin/bash
#
# Added these lines to "sudo crontab"
# sudo crontab -e
# @reboot /home/pi/BitTorrentSync/src/python_code/IP_address_grab.sh
# * * * * * /home/pi/BitTorrentSync/src/python_code/IP_address_grab.sh
#

FILEPATH="/home/pi/BitTorrentSync/src/bash_code"
FILE="rasppi_ip_address.txt"

IP_ADDR="$(hostname -I)"
#echo "${IP_ADDR}"

# Echo Date and IP address to file
cd ${FILEPATH}
sudo echo "$(date)" > $FILE
sudo echo "${IP_ADDR}" >> $FILE

