#!/bin/sh

# Check if file is already patched
grep -q example /etc/nsswitch.conf && exit 0

# Register NSS example plugin in NSS
sed -i '/^passwd/ s/$/ example/' /etc/nsswitch.conf
sed -i '/^group/ s/$/ example/' /etc/nsswitch.conf
