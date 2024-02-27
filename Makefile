# library name
lib.name = \
	structuredDataC

PATH_SRC = src
PATH_GLOBAL = $(PATH_SRC)/global
PATH_CODE = $(PATH_SRC)/structuredData
PATH_SCRIPT = $(PATH_SRC)/sdScript
PATH_PD_OBJS = pd_objs

make-lib-executable=yes

class.sources = \
	$(filter-out $(PATH_CODE)/structuredData.c, $(wildcard $(PATH_CODE)/*.c) $(wildcard $(PATH_SCRIPT)/*.c))

lib.setup.sources = \
	$(PATH_CODE)/structuredData.c

cflags = -Wall -fpic -std=c99 -Winline -fgnu89-inline -I$(PATH_GLOBAL) -I$(PATH_SCRIPT)

# sdScript.class.sources = \
# 	$(PATH_SCRIPT)

datafiles = \
	README.md \
	$(PATH_PD_OBJS)/sdAnyBang.pd \
	$(PATH_PD_OBJS)/sdFloatBang.pd \
	$(PATH_PD_OBJS)/sdSymbolBang.pd \
	$(PATH_PD_OBJS)/sdObjSaveAll.pd \
	$(PATH_PD_OBJS)/sdObjSaveAllToDisk.pd \
	$(PATH_PD_OBJS)/sdObjConnectTo.pd \
	$(PATH_PD_OBJS)/sdObjTrack.pd \
	$(PATH_PD_OBJS)/sdObjTrackAll.pd \
	$(PATH_PD_OBJS)/sdObjGet.pd \
	$(PATH_PD_OBJS)/sdObjGetAll.pd \
	$(PATH_PD_OBJS)/sdObjInterface.pd \
	$(PATH_PD_OBJS)/sdObjInterface_obj.pd \
	$(PATH_PD_OBJS)/sdObjGroupsInterface_obj.pd \
	$(PATH_PD_OBJS)/sdTest.pd \
	$(PATH_PD_OBJS)/gui/sdGui_bool_dyn_props.pd \
	$(PATH_PD_OBJS)/gui/sdGui_bool_obj_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_bool_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_events_dyn_props.pd \
	$(PATH_PD_OBJS)/gui/sdGui_events_obj_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_events_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_num_dyn_props.pd \
	$(PATH_PD_OBJS)/gui/sdGui_num_obj_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_num_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_sym_dyn_props.pd \
	$(PATH_PD_OBJS)/gui/sdGui_sym_obj_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_sym_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_box_helper.pd \
	$(PATH_PD_OBJS)/gui/sdGui_combined_obj_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_combined_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_events_compact_dyn_props.pd \
	$(PATH_PD_OBJS)/gui/sdGui_events_compact_obj_ui.pd \
	$(PATH_PD_OBJS)/gui/sdGui_events_compact_ui.pd \
	$(PATH_PD_OBJS)/utils/sdListForEach.pd \
	$(PATH_PD_OBJS)/math/sdCenterDelta.pd \
	$(PATH_PD_OBJS)/math/sdMinDelta.pd \
	$(PATH_PD_OBJS)/math/sdMinMax.pd \
	$(PATH_PD_OBJS)/math/sdMod.pd \
	$(PATH_PD_OBJS)/dyn_patch/sdBoundingBox.pd \
	$(PATH_PD_OBJS)/dyn_patch/sdDynPatch.pd \
	$(PATH_PD_OBJS)/dyn_patch/sdLines_max.pd \
	$(PATH_PD_OBJS)/dyn_patch/sdLines.pd \
  $(PATH_PD_OBJS)/doc/structuredDataC_doc.pd \
  $(PATH_PD_OBJS)/doc/modules/sdData-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdEvent-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdList-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdObjState-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdObjState_example.pd \
  $(PATH_PD_OBJS)/doc/modules/sdPack-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdUtils-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdPackMatch-help.pd \
  $(PATH_PD_OBJS)/doc/modules/sdMath-help.pd \
	$(PATH_PD_OBJS)/doc/sdScript-help.pd \
	$(PATH_PD_OBJS)/doc/sdObjExample.pd \
	$(PATH_PD_OBJS)/doc/sdObjSndExample.pd \
	$(PATH_PD_OBJS)/doc/sdObjDuplexExample.pd \
	$(PATH_PD_OBJS)/doc/sdObjRcvExample.pd \
	$(PATH_PD_OBJS)/doc/sdObjExampleManyProps.pd

# include pd-lib-builder
PDLIBBUILDER_DIR=dependencies/pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
