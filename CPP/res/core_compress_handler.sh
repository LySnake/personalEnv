#!/bin/sh
RW_DIR=/mnt/UDISK
CORE_DIR=${RW_DIR}/core_file


CORE_FILE=/tmp/core.$2_$1_$3
cat > "$CORE_FILE"

CORE_GZ=${CORE_DIR}/core.$2.gz

gzip -1 ${CORE_FILE} -c > ${CORE_GZ}
sync
rm $CORE_FILE