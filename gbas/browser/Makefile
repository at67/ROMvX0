
CONTRIB_AT67=../../../gigatron-rom/Contrib/at67
TOOLS=${CONTRIB_AT67}/tools
RUNTIME=${CONTRIB_AT67}/gbas/runtime/.
# HW=${CONTRIB_AT67}/hw

PYTHON3=python3
GTBASIC=${TOOLS}/gtbasic/gtbasic
GTHEXDUMP=${PYTHON3} ${TOOLS}/python/gtHexDump.py

all: Browser_SDCard_ROMv2_v1.gt1 Browser_SDCard_ROMv2_v1.h \
     Browser_SDCard_ROMv5a_v1.gt1 Browser_SDCard_ROMv5a_v1.h
ifdef HW
 ifdef OS
	copy Browser_SDCard_ROMv2_v1.h ${HW}/SDCard_NO_ROM/BabelFish
	copy Browser_SDCard_ROMv2_v1.gt1 ${HW}/SDCard_NO_ROM/BabelFish
	copy Browser_SDCard_ROMv5a_v1.gt1  ${HW}/SDCard_ROM
 else
	cp Browser_SDCard_ROMv2_v1.h ${HW}/SDCard_NO_ROM/BabelFish
	cp Browser_SDCard_ROMv2_v1.gt1 ${HW}/SDCard_NO_ROM/BabelFish
	cp Browser_SDCard_ROMv5a_v1.gt1  ${HW}/SDCard_ROM
 endif
endif


%.gbas2: %.gbas
	sed  -e "s|../../runtime|${RUNTIME}|" $< > $@

%.gt1: %.gbas2
	${GTBASIC} $<

%.h: %.gt1
	${GTHEXDUMP} $< > $@


clean: FORCE
ifdef OS
	-del *.h *.gt1 *.gasm
else
	-rm 2>/dev/null  *.h *.gt1 *.gasm
endif

FORCE:

.PHONY: FORCE
