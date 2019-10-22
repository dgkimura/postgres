# contrib/zoit/Makefile

MODULE_big = zoit
OBJS = zoit.o $(WIN32RES)

EXTENSION = zoit
DATA = zoit--1.0.sql
PGFILEDESC = "zoit access method - signature file based index"

REGRESS = oit

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/zoit
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
