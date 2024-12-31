#
# Platform specific options.
#

PKG_CONFIG?=pkg-config

ifdef WINDOWSHELL
rmrf=-2>NUL DEL /S /Q
mkdir=-2>NUL MD
cat=TYPE
else
rmrf=rm -rf
mkdir=mkdir -p
cat=cat
endif

ifndef MAKE_DIR
MAKE_DIR:=Makefile.d/
endif

ifdef ANDROID
LINUX64=1
endif

ifdef LINUX64
LINUX=1
endif

ifdef MINGW64
MINGW=1
endif

ifdef LINUX
UNIX=1
ifdef LINUX64
NONX86=1
# LINUX64 does not imply X86_64=1;
# could mean ARM64 or Itanium
platform=linux/64
else
platform=linux
endif
else ifdef FREEBSD
UNIX=1
platform=freebsd
else ifdef HAIKU
# Give Haiku its own configuration, since it
# isn't actually UNIX.
include Makefile.d/haiku.mk
else ifdef SOLARIS # FIXME: UNTESTED
UNIX=1
platform=solaris
else ifdef CYGWIN32 # FIXME: UNTESTED
platform=cygwin
else ifdef MINGW
ifdef MINGW64
NONX86=1
NOASM=1
# MINGW64 should not necessarily imply X86_64=1,
# but we make that assumption elsewhere
# Once that changes, remove this
X86_64=1
platform=mingw/64
else
platform=mingw
endif
include $(MAKE_DIR)/win32.mk
endif

ifdef platform
makedir:=$(makedir)/$(platform)
endif

ifdef UNIX
include $(MAKE_DIR)/nix.mk
endif

ifeq ($(SDL), 1)
include $(MAKE_DIR)/sdl.mk
else ifeq ($(DEDICATED), 1)
include $(MAKE_DIR)/dedicated.mk
else ifndef ANDROID
include $(MAKE_DIR)/dummy.mk
endif
