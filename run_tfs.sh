#!/bin/bash
echo -e 'TaggableFS'
echo -e 'Santhosh Ranganathan'
echo -e 'CSI680'
echo
echo -e '======================================================================'
echo
echo -e '\e[35mOptions\e[0m'
echo -e '   --log           enable logging'
echo -e '   --tag-view      open filesystem in read-only tag view mode'
echo
echo -e '\e[35mRunning make\e[0m'
make tfs.out
echo
echo -e '\e[35mCreating directories for dummy run\e[0m'
echo 'mkdir TFSmount TFSroot'
mkdir TFSmount TFSroot
echo
echo -e '\e[35mRunning TaggableFS\e[0m'
first=${1:-}
second=${2:-}
echo "./tfs.out --init TFSmount TFSroot $first $second"
./tfs.out --init TFSmount TFSroot $first $second
echo
echo 'If successful, use TFSmount folder to access the mounted filesystem.'
read -n 1 -s -r -p 'Press any key to shutdown TaggableFS...'
echo
echo
echo -n 'Sleeping for 1 second'
echo -n '.'
sleep 0.5
echo '.'
sleep 0.5
echo
echo './tfs.out --shutdown'
./tfs.out --shutdown
echo
echo -n 'Sleeping for 1 second'
echo -n '.'
sleep 0.5
echo '.'
sleep 0.5
echo
echo -e '\e[35mPOSIX Queue(s)\e[0m'
echo -e '\e[36mHelp:\e[0m to mount mqueue, run'
echo -e '    sudo mkdir /dev/mqueue'
echo -e '    sudo mount -t mqueue none /dev/mqueue'
ls -l /dev/mqueue
echo
echo -e '\e[35mtfs.out Process(es) in Memory\e[0m'
echo -e '\e[36mHelp:\e[0m if not responding, try'
echo -e '    kill -9 `pidof tfs.out`; fusermount -u TFSmount'
echo -ne '\e[36m'
ps -efj | head -n 1
echo -ne '\e[0m'
ps -efj | grep "[t]fs.out"
echo
