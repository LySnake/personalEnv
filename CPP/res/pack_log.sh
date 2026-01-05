#!/bin/sh
# Package system logs, configuration files, etc.

RW_DIR=/mnt/UDISK
USER_DIR=/user
DATA_DIR=/data
TMP_DIR=/tmp
APP_DIR=/mnt/app

TMP_PHOTO_DIR=${TMP_DIR}/photo

CORE_DIR=${RW_DIR}/core_file

APP_BIN_DIR=${APP_DIR}/bin
APP_CONFIG_DIR=${APP_DIR}/config

SYS_CONFIG_DIR=${APP_CONFIG_DIR}/system

PUBLIC_PEM=${SYS_CONFIG_DIR}/public.pem

ALIEN_CELL_LOG_DIR=${DATA_DIR}

ENCRYPT_TOOL=${APP_BIN_DIR}/encrypt_tool
META_TOOL=${APP_BIN_DIR}/meta_tool

TAR_GZ_FILE=${TMP_DIR}/acl20d.tar.gz
OUT_ENCRYPT_LOG_FILE=${TAR_GZ_FILE}.sign
META_FILE=${TMP_DIR}/log.meta


FILE_DATE_LOG=${ALIEN_CELL_LOG_DIR}/date.log
FILE_PS_LOG=${ALIEN_CELL_LOG_DIR}/ps.log
FILE_DMESG_LOG=${ALIEN_CELL_LOG_DIR}/dmesg.log
VERSION_FILE=${ALIEN_CELL_LOG_DIR}/os-release

collect_system_info()
{
    ps > ${FILE_PS_LOG}
    dmesg > ${FILE_DMESG_LOG}

    date > ${FILE_DATE_LOG}
    cp /etc/os-release ${VERSION_FILE}
}


collect_system_info
tar -cvf - ${ALIEN_CELL_LOG_DIR} ${USER_DIR} ${CORE_DIR} ${TMP_PHOTO_DIR} | gzip > ${TAR_GZ_FILE}

file_md5=$(md5sum ${TAR_GZ_FILE} | awk '{print $1}')
sign=$(${ENCRYPT_TOOL} ${TAR_GZ_FILE} --output ${OUT_ENCRYPT_LOG_FILE} --key "${file_md5}" --pem ${PUBLIC_PEM})
meta_json='{ "file_type": "tar.gz", "sign": "'"$sign"'" }'

echo ${meta_json} > ${META_FILE}
${META_TOOL} --metafile ${META_FILE} --in-place ${OUT_ENCRYPT_LOG_FILE}


rm -rf ${TAR_GZ_FILE}
rm -rf ${META_FILE}

