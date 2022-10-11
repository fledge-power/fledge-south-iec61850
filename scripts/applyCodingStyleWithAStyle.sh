#!/bin/sh

#set -x

ASTYLE=$(which astyle)

if [ -z "${ASTYLE}" ]; then
    echo "Astyle not found. Please install Astyle."
    exit 1
else
    ROOT_DIR_PROJECT="$(realpath $(dirname $0)/../)"
    cd ${ROOT_DIR_PROJECT}
    ${ASTYLE} --style=kr \
              --indent-classes --indent-switches \
              --pad-oper --pad-comma --pad-header \
              --delete-empty-lines --break-blocks \
              --align-pointer=name --align-reference=name --add-braces \
              --recursive --exclude=${ROOT_DIR_PROJECT}/build --suffix=none *.cpp,*.h
fi
