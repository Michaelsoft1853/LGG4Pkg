#
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.
#  Copyright (c) 2018, Bingxing Wang. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = LGG4
  PLATFORM_GUID                  = c7436bd3-ae4e-4b2e-b230-bd8c46eeef71
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/LGG4-$(ARCH)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platforms/LGG4/LGG4.fdf

  DEFINE SECURE_BOOT_ENABLE           = TRUE
  DEFINE USE_SCREEN_FOR_SERIAL_OUTPUT = TRUE
  DEFINE USE_UART_FOR_SERIAL_OUTPUT   = TRUE
  DEFINE MEMORY_4GB                   = 0

[BuildOptions.common]
  GCC:*_*_AARCH64_CC_FLAGS = -DSILICON_PLATFORM=8992 -DDEVICE_LGG4=1

[PcdsFixedAtBuild.common]
  # Platform-specific
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x00000000         # 0GB Base
  gArmTokenSpaceGuid.PcdSystemMemorySize|0xC0000000         # 3GB Size
  gArmPlatformTokenSpaceGuid.PcdCoreCount|6
  gArmPlatformTokenSpaceGuid.PcdClusterCount|2
  gNexus5XPkgTokenSpaceGuid.PcdSmbiosSystemModel|"LG G4"
  gNexus5XPkgTokenSpaceGuid.PcdSmbiosProcessorModel|"Qualcomm Snapdragon 808 Processor (8992)"
  gNexus5XPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"H815"
  gNexus5XPkgTokenSpaceGuid.SynapticsXMax|1440
  gNexus5XPkgTokenSpaceGuid.SynapticsYMax|2560
  gNexus5XPkgTokenSpaceGuid.PsciCpuSuspendAddress|0x6c03920

  gNexus5XPkgTokenSpaceGuid.PcdMipiFrameBufferAddress|0x03400000
  gNexus5XPkgTokenSpaceGuid.PcdMipiFrameBufferWidth|1440
  gNexus5XPkgTokenSpaceGuid.PcdMipiFrameBufferHeight|2560
  gNexus5XPkgTokenSpaceGuid.PcdMipiFrameBufferPixelBpp|32

  # Device Driver Synaptics 3202
  gNexus5XPkgTokenSpaceGuid.SynapticsCtlrAddress|0x20
  gNexus5XPkgTokenSpaceGuid.SynapticsCtlrResetPin|8
  gNexus5XPkgTokenSpaceGuid.SynapticsCtlrIntPin|22
  gNexus5XPkgTokenSpaceGuid.SynapticsCtlrI2cDevice|2

[PcdsDynamicDefault.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|1440
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|2560
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|1440
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|2560
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutRow|120
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutColumn|160
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|120
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|160
  
!include Nexus5XPkg/Shared.dsc.inc
