#!/system/bin/sh
DEBUG=false
MODDIR=${0%/*}

for PROP in $(resetprop | grep -oE 'ro.*.build.type'); do
    resetprop_if_diff "$PROP" user
done

grep "APatch" /proc/mounts | while read line; do
    # Extract the mount points
    mount_point=$(echo $line | awk '{print $2}')
    if [[ "$mount_point" != "/system*" ]] && [[ "$mount_point" != "/product*" ]]; then
        echo "Unmounting $mount_point..."
        # Try to unmount it
        umount -l $mount_point
        if [ $? -eq 0 ]; then
            echo "$mount_point successfully unmounted."
        else
            echo "Failed to unmount $mount_point."
        fi
    fi
done
