TOPDIR	:= ../../..
MKDIR	:= $(TOPDIR)/build/makefiles
SRCDIR	:= $(TOPDIR)/src

FLAVOR	?= $(error FLAVOR not set)

include $(MKDIR)/$(FLAVOR).mk
