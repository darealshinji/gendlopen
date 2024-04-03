# This one Makefile works with Microsoft nmake and GNU make.
# They use different conditional syntax, but each can be
# nested and inverted within the other.

all: default


ifdef MAKEDIR: # gmake: false; nmake: unused target
!ifdef MAKEDIR # gmake: not seen; nmake: true

#
# Microsoft nmake
#

!include ..\nmake.mk


GENDLOPEN = ..\src\gendlopen.exe


default: uuid_win.exe

clean:
	-del /q uuid_win.exe uuid_win_dl.h *.o *.obj

uuid_win.exe: uuid_win_dl.h
	$(CL) $(CFLAGS) $*.c -Fe$@ $(LDFLAGS)


!else # and now the other
else

#
# GNU make
#

include ../gmake.mk


ifneq ($(CLANG_CL),)
# clang-cl
BINS = uuid_win
else

ifneq ($(shell $(CC) -dumpmachine | grep mingw),)
# MinGW
BINS = uuid_win

else

# other/ELF
BINS       = uuid_nix notify appindicator
CFLAGS    += -Wno-deprecated-declarations
#LIBDL     = -ldl
LDFLAGS   += $(LIBDL) -Wl,-rpath,'$$ORIGIN'

ifneq ($(shell $(CC) -dumpmachine | grep gnu),)
CFLAGS    += -D_GNU_SOURCE
CXXFLAGS  += -D_GNU_SOURCE
endif

endif # MINGW
endif # CLANG_CL


GENDLOPEN := ../src/gendlopen


.PHONY: all default clean

default: $(BINS)

clean:
	-rm -f $(BINS) notify appindicator uuid_nix
	-rm -f *.exe *.o *.obj example_*.h uuid_nix_dl.h uuid_win_dl.h


# appindicator

example_appindicator.h: appindicator.txt
	$(GENDLOPEN) -i $^ -f -o $@ -n xappindicator --library libappindicator.so.1 --skip-parameter-names

example_appindicator_gtk.h: appindicator_gtk.txt
	$(GENDLOPEN) -i $^ -f -o $@ -n xgtk --library libgtk-x11-2.0.so.0

example_appindicator_gobject.h: appindicator_gobject.txt
	$(GENDLOPEN) -i $^ -f -o $@ -n xgobject --library libgobject-2.0.so.0

appindicator: example_appindicator.h example_appindicator_gobject.h example_appindicator_gtk.h
	$(CC) $(CFLAGS) -DUSE_DLOPEN=1 -Ithirdparty `pkg-config --cflags gtk+-2.0 gobject-2.0` \
	  appindicator.c -o $@ $(LDFLAGS)


# libnotify

example_notify.h: notify.txt
	$(GENDLOPEN) -i $^ -f -o $@ -n xnotify --library="api:4:notify"

example_notify_gobject.h: notify_gobject.txt
	$(GENDLOPEN) -i $^ -f -o $@ -n xgobject --library="api:0:gobject-2.0"

notify: example_notify.h example_notify_gobject.h
	$(CC) $(CFLAGS) -DUSE_DLOPEN=1 -Ithirdparty `pkg-config --cflags gobject-2.0` \
	  notify.c -o $@ $(LDFLAGS)


# UUID

uuid_nix_dl.h: uuid_nix.h
	$(CLANG) -Xclang -ast-dump -Ithirdparty/uuid $< | \
	  $(GENDLOPEN) -i- --library libuuid.so.1 \
	  -Suuid_generate_random -Suuid_is_null -Suuid_unparse_lower \
	  -f -o $@

uuid_nix: uuid_nix_dl.h
	$(CC) $(CFLAGS) -Ithirdparty/uuid uuid_nix.c -o $@ $(LDFLAGS)

uuid_win: uuid_win_dl.h
	$(CC) $(CFLAGS) -Ithirdparty/uuid uuid_win.c -o $@ $(LDFLAGS)


endif    # gmake: close condition; nmake: not seen
!endif : # gmake: unused target; nmake close conditional


# common target
uuid_win_dl.h: uuid_win.h
	$(CLANG) -Xclang -ast-dump -fansi-escape-codes uuid_win.h | \
	  $(GENDLOPEN) -i- --library rpcrt4.dll \
	  -SUuidCreate -SUuidToStringA -SRpcStringFreeA -f -o $@

