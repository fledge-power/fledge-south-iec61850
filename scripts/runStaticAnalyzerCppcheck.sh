#!/bin/sh

#set -x

CPPCHECK=$(which cppcheck)

if [ -z "${CPPCHECK}" ]; then
    echo "cppcheck not found. Please install cppcheck."
    exit 1
fi

if [ -z "${FLEDGE_ROOT}" ]; then
    echo "FLEDGE_ROOT environment variable is not set: exit."
    exit 1
fi


ROOT_DIR_PROJECT="$(realpath $(dirname $0)/../)"
cd ${ROOT_DIR_PROJECT}

${CPPCHECK} --enable=all --inconclusive  \
    --suppress=*:$FLEDGE_ROOT/C/common/* \
    -I ${ROOT_DIR_PROJECT}/include/ -I ${ROOT_DIR_PROJECT}/build -I /usr/local/include/ \
    -I $FLEDGE_ROOT/C/common/include/ \
    -I $FLEDGE_ROOT/C/services/common/include/ \
    --template='{file}({line}):({severity}) {message}' ${ROOT_DIR_PROJECT}/*cpp
