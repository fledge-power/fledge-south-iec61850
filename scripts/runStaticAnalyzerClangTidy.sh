#!/bin/sh

#set -x

CLANG_TIDY=$(which clang-tidy)

if [ -z "${CLANG_TIDY}" ]; then
    echo "clang-tidy not found. Please install clang-tidy."
    exit 1
fi

if [ -z "${FLEDGE_ROOT}" ]; then
    echo "FLEDGE_ROOT environment variable is not set: exit."
    exit 1
fi


ROOT_DIR_PROJECT="$(realpath $(dirname $0)/../)"
TMP_OUTPUT_FILE="/tmp/${USER}_clang-tidy_output.log"

cd ${ROOT_DIR_PROJECT}
rm -f ${TMP_OUTPUT_FILE}

find ${ROOT_DIR_PROJECT} -not -path "*build*" -name "*.cpp" -exec ${CLANG_TIDY} {} \
-header-filter="${ROOT_DIR_PROJECT}*" \
-checks=*,clang-analyzer-*,clang-analyzer-cplusplus*,\
-fuchsia*,-llvmlibc*,\
-modernize-use-trailing-return-type \
-- -I ${ROOT_DIR_PROJECT}/include/ -I ${FLEDGE_ROOT}/C/common/include/ \; >> ${TMP_OUTPUT_FILE}


# show a light version of the warning list
LIGHT_LIST=$(grep -E "warning:|error:" ${TMP_OUTPUT_FILE} | sed "s#${ROOT_DIR_PROJECT}##" | sort -u)

ERROR_COUNT=$(echo "${LIGHT_LIST}" | wc -l)

echo "${LIGHT_LIST}"

echo
echo " => Error/Warning count: ${ERROR_COUNT}"
echo " (more details in the clang-tidy output file: ${TMP_OUTPUT_FILE})"
