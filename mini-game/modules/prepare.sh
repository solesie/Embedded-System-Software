echo "7 6 1 7" > /proc/sys/kernel/printk
rmmod /data/local/tmp/game1.ko
insmod /data/local/tmp/game1.ko
chmod 777 /dev/game1