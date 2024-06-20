echo "7 6 1 7" > /proc/sys/kernel/printk
rmmod /data/local/tmp/minigame.ko
insmod /data/local/tmp/minigame.ko
chmod 777 /dev/minigame