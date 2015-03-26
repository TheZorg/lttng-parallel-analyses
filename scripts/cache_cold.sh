#!/bin/bash

# Runs the given command after dropping pagecache
echo 1 > /proc/sys/vm/drop_caches
$@
