cmake_minimum_required(VERSION 3.5)

set(CORE_SRCS 
src/SdCard/SdSpiCard.cpp
src/SpiDriver/SdSpiESP.cpp
src/SpiDriver/SdSpiChipSelect.cpp
src/FsLib/FsNew.cpp
src/FsLib/FsFile.cpp
src/FsLib/FsVolume.cpp
src/FatLib/FatDbg.cpp
src/FatLib/FatFile.cpp
src/FatLib/FatFileLFN.cpp
src/FatLib/FatFilePrint.cpp
src/FatLib/FatFileSFN.cpp
src/FatLib/FatFormatter.cpp
src/FatLib/FatName.cpp
src/FatLib/FatPartition.cpp
src/FatLib/FatVolume.cpp
src/common/FmtNumber.cpp
src/common/FsCache.cpp
src/common/FsDateTime.cpp
src/common/FsName.cpp
src/common/FsStructs.cpp
src/common/FsUtf.cpp
src/common/upcase.cpp
src/ExFatLib/ExFatFile.cpp
src/ExFatLib/ExFatFileWrite.cpp
src/ExFatLib/ExFatDbg.cpp
src/ExFatLib/ExFatFile.cpp
src/ExFatLib/ExFatFilePrint.cpp
src/ExFatLib/ExFatFileWrite.cpp
src/ExFatLib/ExFatFormatter.cpp
src/ExFatLib/ExFatName.cpp
src/ExFatLib/ExFatPartition.cpp
src/ExFatLib/ExFatVolume.cpp
)

set(includedirs
src/.
src/SdCard/.
src/SpiDriver/.
src/FsLib/.
src/FatLib/.
src/ExFatLib/.
src/FatLib/.
src/common/.
)

set(requires arduino)

idf_component_register(INCLUDE_DIRS ${includedirs} SRCS ${CORE_SRCS} REQUIRES ${requires})
