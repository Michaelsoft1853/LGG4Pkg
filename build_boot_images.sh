#!/usr/bin/bash
set -e

device="${1}"
build_type="${2:-DEBUG}"
devicelower="$(tr '[:upper:]' '[:lower:]' <<< "${device}")"

case "${device}" in
Nexus6P)
ARCH_DIR="Nexus6P-AARCH64"
EFI_FD_NAME="MSM8994_EFI.fd"
DTB="ImageResources/Angler/msm8994-huawei-angler.dtb"
OUT_IMG="ImageResources/Angler/uefi_angler.img"
PAGESIZE=4096
RAMDISK_OFFSET="0x02000000"
TAGS_OFFSET="0x01e00000"
    ;;
Nexus5X)
ARCH_DIR="Nexus5X-AARCH64"
EFI_FD_NAME="MSM8992_EFI.fd"
DTB="ImageResources/Bullhead/msm8992_lge_bullhead.dtb"
OUT_IMG="ImageResources/Bullhead/uefi_bullhead.img"
PAGESIZE=4096
RAMDISK_OFFSET="0x02000000"
TAGS_OFFSET="0x01e00000"
    ;;
LGG4)
ARCH_DIR="LGG4-AARCH64"
EFI_FD_NAME="MSM8992_EFI.fd"
DTB="ImageResources/LGG4/msm8992_lge_g4.dtb"
OUT_IMG="ImageResources/LGG4/uefi_lgg4.img"
PAGESIZE=4096
RAMDISK_OFFSET="0x01000000"
TAGS_OFFSET="0x00000100"
    ;;
*)
echo "Bootimages: Invalid platform: ${device}" 1>&2
exit 1
    ;;
esac

FV_DIR="workspace/Build/${ARCH_DIR}/${build_type}_GCC5/FV"
EFI_FD="${FV_DIR}/${EFI_FD_NAME}"

if [ ! -f "${EFI_FD}" ]; then
echo "Unable to find build artifacts (${EFI_FD})." 1>&2
exit 1
fi

cat "BootShim/BootShim.bin" "${EFI_FD}" > "${FV_DIR}/BootShim${EFI_FD_NAME}"
gzip -c < "${FV_DIR}/BootShim${EFI_FD_NAME}" > "${FV_DIR}/BootShim${EFI_FD_NAME}.gz"
cat "${FV_DIR}/BootShim${EFI_FD_NAME}.gz" "${DTB}" > "${FV_DIR}/Image.gz-dtb"

ramdisk="$(mktemp)"
printf "\0" > "${ramdisk}"

python3 "ImageResources/mkbootimg.py" \
--kernel "${FV_DIR}/Image.gz-dtb" \
--ramdisk "${ramdisk}" \
--base 0x00000000 \
--pagesize "${PAGESIZE}" \
--ramdisk_offset "${RAMDISK_OFFSET}" \
--tags_offset "${TAGS_OFFSET}" \
-o "${OUT_IMG}"
ret=$?

if [ "${device}" == "LGG4" ]; then
    python3 "ImageResources/mkbootimg.py" \
    --kernel "${FV_DIR}/Image.gz-dtb" \
    --ramdisk "${ramdisk}" \
    --base 0x00000000 \
    --pagesize 2048 \
    --ramdisk_offset "${RAMDISK_OFFSET}" \
    --tags_offset "${TAGS_OFFSET}" \
    -o "ImageResources/LGG4/uefi_lgg4_2k.img"
    echo "Built ImageResources/LGG4/uefi_lgg4_2k.img (2048 pagesize)"
fi

rm -f "${ramdisk}"

if [ ${ret} -ne 0 ]; then
echo "Failed to build ${OUT_IMG}" 1>&2
exit 1
fi

echo "Built ${OUT_IMG}"