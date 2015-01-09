#!/bin/sh
exec make -j`getconf _NPROCESSORS_ONLN || echo 1` $@
