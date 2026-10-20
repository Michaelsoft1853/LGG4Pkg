#include <Library/PcdLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmSmcLib.h>
#include <Library/PlatformPrePiLib.h>

#include "PlatformUtils.h"
#include <Configuration/DeviceMemoryMap.h>

#include <Chipset/mdp5.h>

#include <Platform/iomap.h>

EFI_STATUS
EFIAPI
SerialPortLocateArea(PARM_MEMORY_REGION_DESCRIPTOR_EX* MemoryDescriptor)
{
  PARM_MEMORY_REGION_DESCRIPTOR_EX MemoryDescriptorEx =
      gDeviceMemoryDescriptorEx;

  // Run through each memory descriptor
  while (MemoryDescriptorEx->Length != 0) {
    if (AsciiStriCmp("PStore", MemoryDescriptorEx->Name) == 0) {
      *MemoryDescriptor = MemoryDescriptorEx;
      return EFI_SUCCESS;
    }
    MemoryDescriptorEx++;
  }

  return EFI_NOT_FOUND;
}

VOID InitializeSharedUartBuffers(VOID)
{
#if USE_MEMORY_FOR_SERIAL_OUTPUT == 1
  PARM_MEMORY_REGION_DESCRIPTOR_EX PStoreMemoryRegion = NULL;
#endif

  INTN* pFbConPosition = (INTN*)(FixedPcdGet32(PcdMipiFrameBufferAddress) + (FixedPcdGet32(PcdMipiFrameBufferWidth) * 
                                                                              FixedPcdGet32(PcdMipiFrameBufferHeight) * 
                                                                              FixedPcdGet32(PcdMipiFrameBufferPixelBpp) / 8));

  *(pFbConPosition + 0) = 0;
  *(pFbConPosition + 1) = 0;

#if USE_MEMORY_FOR_SERIAL_OUTPUT == 1
  // Clear PStore area
  SerialPortLocateArea(&PStoreMemoryRegion);
  UINT8 *base = (UINT8 *)PStoreMemoryRegion->Address;
  for (UINTN i = 0; i < PStoreMemoryRegion->Length; i++) {
    base[i] = 0;
  }
#endif
}

STATIC VOID UartInit(VOID)
{
  SerialPortInitialize();
  InitializeSharedUartBuffers();

#if defined(DEVICE_LGG4)
  DEBUG((EFI_D_INFO, "\nTianoCore on LG G4 (AArch64)\n"));
#elif SILICON_PLATFORM == 8992
  DEBUG((EFI_D_INFO, "\nTianoCore on Nexus 5X (AArch64)\n"));
#else
  DEBUG((EFI_D_INFO, "\nTianoCore on Nexus 6P (AArch64)\n"));
#endif
  DEBUG(
      (EFI_D_INFO, "Firmware version %s built %a %a\n\n",
       (CHAR16 *)PcdGetPtr(PcdFirmwareVersionString), __TIME__, __DATE__));
}

typedef struct {
  CONST CHAR8 *Name;
  UINT32       Base;
  UINT32       FlushBit;
  BOOLEAN      IsVig;
} MDP_PIPE_DESCRIPTOR;

typedef struct {
  BOOLEAN WasActive;
  UINT32  OldSize;
  UINT32  OldAddr;
  UINT32  OldStride;
  UINT32  OldFormat;
  UINT32  NewAddr;
  UINT32  NewStride;
  UINT32  NewFormat;
} MDP_PIPE_RECORD;

STATIC CONST MDP_PIPE_DESCRIPTOR mMdpPipes[] = {
  { "VIG0", 0xFD905000, BIT(0),  TRUE  },
  { "VIG1", 0xFD907000, BIT(1),  TRUE  },
  { "VIG2", 0xFD909000, BIT(2),  TRUE  },
  { "RGB0", 0xFD915000, BIT(3),  FALSE },
  { "RGB1", 0xFD917000, BIT(4),  FALSE },
  { "RGB2", 0xFD919000, BIT(5),  FALSE },
  { "DMA0", 0xFD925000, BIT(11), FALSE },
  { "DMA1", 0xFD927000, BIT(12), FALSE },
};

#define MDP_PIPE_COUNT (sizeof(mMdpPipes) / sizeof(mMdpPipes[0]))

STATIC MDP_PIPE_RECORD mPipeRecords[MDP_PIPE_COUNT];

/* 
 * Reconfigure the framebuffer to a BGRA 32bpp one (windows requirements)
 * Dynamically detects active pipes configured by Little Kernel and updates
 * their format, unpack pattern, and stride to 32bpp BGRA8888.
 */
VOID CheckMdpConfig(VOID)
{
  UINT32 Width        = FixedPcdGet32(PcdMipiFrameBufferWidth);
  UINT32 FbBase       = FixedPcdGet32(PcdMipiFrameBufferAddress);
  UINT32 TargetStride = Width * 4;
  UINT32 FlushMask    = 0;
  UINTN  i;

  ZeroMem(mPipeRecords, sizeof(mPipeRecords));

  for (i = 0; i < MDP_PIPE_COUNT; i++) {
    UINT32 PipeBase  = mMdpPipes[i].Base;
    UINT32 SrcSize   = MmioRead32(PipeBase + PIPE_SSPP_SRC_SIZE);
    UINT32 SrcAddr   = MmioRead32(PipeBase + PIPE_SSPP_SRC0_ADDR);
    UINT32 SrcStride = MmioRead32(PipeBase + PIPE_SSPP_SRC_YSTRIDE);
    UINT32 SrcFormat = MmioRead32(PipeBase + PIPE_SSPP_SRC_FORMAT);

    /* An active pipe configured by Little Kernel has non-zero size, non-zero stride,
     * and a source address pointing to system DDR memory in the framebuffer area.
     */
    if (SrcSize != 0 && SrcStride != 0 && SrcAddr >= 0x01000000 && SrcAddr < 0x80000000) {
      UINT32 NewFormat = mMdpPipes[i].IsVig ? 0x000237FF : 0x000236FF;
      UINT32 NewAddr   = SrcAddr;

      /* If LK configured this pipe with an address offset for split display (e.g. right half),
       * the offset was in 24bpp (3 bytes per pixel). In 32bpp, convert it to 4 bytes per pixel.
       */
      if (SrcAddr > FbBase && SrcAddr < FbBase + (Width * 4)) {
        UINT32 Offset = SrcAddr - FbBase;
        if ((Offset % 3) == 0) {
          UINT32 Pixels = Offset / 3;
          NewAddr = FbBase + (Pixels * 4);
          MmioWrite32(PipeBase + PIPE_SSPP_SRC0_ADDR, NewAddr);
        }
      }

      /* Reconfigure SSPP pipe to 32bpp BGRA8888 */
      MmioWrite32(PipeBase + PIPE_SSPP_SRC_FORMAT, NewFormat);
      MmioWrite32(PipeBase + PIPE_SSPP_SRC_UNPACK_PATTERN, 0x03020001);
      MmioWrite32(PipeBase + PIPE_SSPP_SRC_YSTRIDE, TargetStride);

      FlushMask |= mMdpPipes[i].FlushBit;

      mPipeRecords[i].WasActive = TRUE;
      mPipeRecords[i].OldSize   = SrcSize;
      mPipeRecords[i].OldAddr   = SrcAddr;
      mPipeRecords[i].OldStride = SrcStride;
      mPipeRecords[i].OldFormat = SrcFormat;
      mPipeRecords[i].NewAddr   = NewAddr;
      mPipeRecords[i].NewStride = TargetStride;
      mPipeRecords[i].NewFormat = NewFormat;
    }
  }

  if (FlushMask != 0) {
    MmioWrite32(MDP_CTL_0_BASE + CTL_FLUSH, FlushMask);
#if defined(DEVICE_LGG4) || SILICON_PLATFORM == 8994
    MmioWrite32(MDP_CTL_1_BASE + CTL_FLUSH, FlushMask);
#endif
  } else {
    /* Fallback if no pipes were dynamically detected */
#if SILICON_PLATFORM == 8992 && !defined(DEVICE_LGG4)
    MmioWrite32(BULLHEAD_PIPE_BASE + PIPE_SSPP_SRC_FORMAT, 0x000236FF);
    MmioWrite32(BULLHEAD_PIPE_BASE + PIPE_SSPP_SRC_UNPACK_PATTERN, 0x03020001);
    MmioWrite32(BULLHEAD_PIPE_BASE + PIPE_SSPP_SRC_YSTRIDE, Width * 4);
    MmioWrite32(MDP_CTL_0_BASE + CTL_FLUSH, (1 << (3)));
#else
    MmioWrite32(ANGLER_PIPE_BASE + PIPE_SSPP_SRC_FORMAT, 0x000237FF);
    MmioWrite32(ANGLER_PIPE_BASE + PIPE_SSPP_SRC_UNPACK_PATTERN, 0x03020001);
    MmioWrite32(ANGLER_PIPE_BASE + PIPE_SSPP_SRC_YSTRIDE, Width * 4);
    MmioWrite32(MDP_CTL_0_BASE + CTL_FLUSH, (1 << (0)));

    MmioWrite32(ANGLER_PIPE2_BASE + PIPE_SSPP_SRC_FORMAT, 0x000237FF);
    MmioWrite32(ANGLER_PIPE2_BASE + PIPE_SSPP_SRC_UNPACK_PATTERN, 0x03020001);
    MmioWrite32(ANGLER_PIPE2_BASE + PIPE_SSPP_SRC_YSTRIDE, Width * 4);
    MmioWrite32(MDP_CTL_1_BASE + CTL_FLUSH, (1 << (1)));
#endif
  }
}

STATIC VOID ReportMdpConfig(VOID)
{
  UINTN i;
  UINTN ActiveCount = 0;

  for (i = 0; i < MDP_PIPE_COUNT; i++) {
    if (mPipeRecords[i].WasActive) {
      ActiveCount++;
      DEBUG((EFI_D_INFO,
        "[MDP] Active pipe %a (0x%08X): %dx%d, Addr 0x%08X->0x%08X, Stride %d->%d, Fmt 0x%08X\n",
        mMdpPipes[i].Name,
        mMdpPipes[i].Base,
        mPipeRecords[i].OldSize & 0xFFFF,
        mPipeRecords[i].OldSize >> 16,
        mPipeRecords[i].OldAddr,
        mPipeRecords[i].NewAddr,
        mPipeRecords[i].OldStride,
        mPipeRecords[i].NewStride,
        mPipeRecords[i].OldFormat));
    }
  }

  DEBUG((EFI_D_INFO, "[MDP] Reconfigured %d pipe(s) to 32bpp BGRA8888\n\n", ActiveCount));
}

/* 
 * Turn on autorefresh on command-mode panels 
 * (stock bootloader disables it here).
 *
 * Based on lk2nd code :
 * https://github.com/msm8916-mainline/lk2nd/pull/137/commits/d0492fdd1f5e6ff51a181fd6d4920c62adc1a23e#diff-c75b4efb45794d329b2414b12f0a97190c5fe0d9dd43f33e84969e1c34f5ca99R158
 */
STATIC
VOID
DisplayEnableRefresh(VOID)
{
#if SILICON_PLATFORM == 8994
  UINT32 Height = FixedPcdGet32(PcdMipiFrameBufferHeight);
  UINT32 VsyncCount = 19200000 / (Height * 60);
#endif
  UINT32 MdssMdpRev = MmioRead32(MDP_HW_REV);
  UINT32 pp0_base;
  UINT32 pp1_base;

  if (MdssMdpRev >= MDSS_MDP_HW_REV_105) {
    pp0_base = REG_MDP(0x71000);
    pp1_base = REG_MDP(0x71800);
  } else if (MdssMdpRev >= MDSS_MDP_HW_REV_102) {
    pp0_base = REG_MDP(0x12D00);
    pp1_base = REG_MDP(0x12E00);
  } else {
    pp0_base = REG_MDP(0x21B00);
    pp1_base = REG_MDP(0x21C00);
  }

#if SILICON_PLATFORM == 8994
  /* Only needed for angler, LG bootloader has it enabled already */
  MmioWrite32(pp0_base + MDP_PP_SYNC_CONFIG_VSYNC, VsyncCount | BIT(19));
#endif

  /* Enable autorefresh and flush */
  MmioWrite32(pp0_base + MDP_PP_AUTOREFRESH_CONFIG, BIT(31) | AUTOREFRESH_FRAMENUM);
#if defined(DEVICE_LGG4)
  MmioWrite32(pp1_base + MDP_PP_AUTOREFRESH_CONFIG, BIT(31) | AUTOREFRESH_FRAMENUM);
#endif
  MmioWrite32(MDP_CTL_0_BASE + CTL_START, 1);
#if defined(DEVICE_LGG4)
  MmioWrite32(MDP_CTL_1_BASE + CTL_START, 1);
#endif
}

VOID PlatformInitialize()
{
  /* 1. Reconfigure hardware MDP display pipes from LK's 24bpp RGB888 to 32bpp BGRA8888 */
  CheckMdpConfig();

  /* 2. Enable continuous display refresh for command-mode panels */
  DisplayEnableRefresh();

  /* 3. Initialize screen serial console (clears screen with 32bpp dark navy blue) */
  UartInit();

  /* 4. Print MDP pipe configuration report so it renders crisply on screen */
  ReportMdpConfig();
}
