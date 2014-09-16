#
#    This file is part of SourceOP.
#
#    SourceOP is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    SourceOP is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
#


# Sample server plugin for SRC engine
#
# October 2004, alfred@valvesoftware.com
#

# From main makefile
#======================================================
#
NAME=server

GAME_DIR=~/steampipe/tf

CPLUS=/usr/bin/g++
CLINK=/usr/bin/gcc
#CPP_LIB=/usr/lib/libstdc++.a /usr/lib/gcc/i586-suse-linux/4.1.2/libgcc_eh.a
#CPP_LIB=/usr/lib/gcc/i386-redhat-linux/4.1.2/libstdc++.a /usr/lib/gcc/i386-redhat-linux/4.1.2/libgcc_eh.a

#LDFLAGS=-m32 -lm -ldl -lssl tier0_i486.so vstdlib_i486.so
LDFLAGS=-m32 -lm -ldl -lssl libtier0_srv.so libvstdlib_srv.so

SOURCE_DIR=/home/apaloma/hl2sdk_smob
BUILD_DIR=/home/apaloma/hl2sdk_smob/utils/AdminOP
RELEASE_DIR=./Release
DEBUG_DIR=./Debug
BUILD_OBJ_DIR_DEBUG=$(DEBUG_DIR)
BUILD_OBJ_DIR_RELEASE=$(RELEASE_DIR)
ifeq "$(DEBUGBUILD)" "true"
	BUILD_OBJ_DIR=$(BUILD_OBJ_DIR_DEBUG)
else
	BUILD_OBJ_DIR=$(BUILD_OBJ_DIR_RELEASE)
endif

ARCH=i486
ARCH_CFLAGS=-mtune=i686 -march=pentium3 -mmmx -O3 -m32

# -fpermissive is so gcc 3.4.x doesn't complain about some template stuff
BASE_CFLAGS=-fpermissive -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -DNO_MALLOC_OVERRIDE
SHLIBEXT=so
SHLIBCFLAGS=-fPIC
#SHLIBLDFLAGS=-shared -Wl,-Map,$@_map.txt -Wl
SHLIBLDFLAGS=-shared -static-libgcc

CFLAGS=$(USER_CFLAGS) $(DEFINES) $(ARCH_CFLAGS) $(BASE_CFLAGS) -Usprintf=use_Q_snprintf_instead_of_sprintf -Ustrncpy=use_Q_strncpy_instead -UPROTECTED_THINGS_ENABLE

BASE_DEFINES=CC=$(CC) CPLUS=$(CPLUS) CPP_LIB=$(CPP_LIB) \
     	BUILD_DIR=$(BUILD_DIR) BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) \
  	SOURCE_DIR=$(SOURCE_DIR) SHLIBLDFLAGS=$(SHLIBLDFLAGS) SHLIBEXT=$(SHLIBEXT) \
  	CLINK=$(CLINK) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) \
      	ARCH=$(ARCH) GAME_DIR=$(GAME_DIR) MOD_CONFIG=$(MOD_CONFIG) NAME=$(NAME) \
     	XERCES_INC_DIR=$(XERCES_INC_DIR) XERCES_LIB_DIR=$(XERCES_LIB_DIR)

#
#======================================================
#

PLUGIN_SRC_DIR=$(SOURCE_DIR)/utils/AdminOP
PUBLIC_SRC_DIR=$(SOURCE_DIR)/public
TIER0_PUBLIC_SRC_DIR=$(SOURCE_DIR)/public/tier0
MYSQL_INC_DIR=/usr/include/mysql

PLUGIN_OBJ_DIR=$(BUILD_OBJ_DIR)/plugin
PLUGIN_OBJ_DIR_RELEASE=$(BUILD_OBJ_DIR_RELEASE)/plugin
PLUGIN_OBJ_DIR_DEBUG=$(BUILD_OBJ_DIR_DEBUG)/plugin
PUBLIC_OBJ_DIR=$(BUILD_OBJ_DIR)/plugin/public

#debug
ifeq "$(DEBUGBUILD)" "true"
	DEBUG = -g -ggdb
	CFLAGS+= $(DEBUG)
endif

INCLUDEDIRS=-I$(PUBLIC_SRC_DIR) -I$(PUBLIC_SRC_DIR)/engine -I$(PUBLIC_SRC_DIR)/tier0 -I$(PUBLIC_SRC_DIR)/tier1 -I$(PUBLIC_SRC_DIR)/mathlib -I$(PUBLIC_SRC_DIR)/game/server -I$(SOURCE_DIR)/game/shared -I$(SOURCE_DIR)/game/server -I$(SOURCE_DIR)/common -I$(PLUGIN_SRC_DIR)/lua/include -I${PLUGIN_SRC_DIR}/mmsource -I${PLUGIN_SRC_DIR}/mmsource/sourcehook -I$(MYSQL_INC_DIR) -Dstrcmpi=strcasecmp -D_alloca=alloca

DO_CC=nice -n 20 $(CPLUS) $(INCLUDEDIRS) -w $(CFLAGS) -DARCH=$(ARCH) -o $@ -c $<

#####################################################################

PLUGIN_OBJS = \
	$(PLUGIN_OBJ_DIR)/mmsource/sourcehook/sourcehook.o \
	$(PLUGIN_OBJ_DIR)/mmsource/sourcehook/sourcehook_hookmangen.o \
	$(PLUGIN_OBJ_DIR)/admincommands.o \
	$(PLUGIN_OBJ_DIR)/AdminOP.o \
	$(PLUGIN_OBJ_DIR)/adminopplayer.o \
	$(PLUGIN_OBJ_DIR)/bans.o \
	$(PLUGIN_OBJ_DIR)/bone_setup.o \
	$(PLUGIN_OBJ_DIR)/bytebuf.o \
	$(PLUGIN_OBJ_DIR)/CAOPEntity.o \
	$(PLUGIN_OBJ_DIR)/CAOPExploding.o \
	$(PLUGIN_OBJ_DIR)/CAOPFragTest.o \
	$(PLUGIN_OBJ_DIR)/CAOPGrenTimer.o \
	$(PLUGIN_OBJ_DIR)/CAOPLuaNormal.o \
	$(PLUGIN_OBJ_DIR)/CAOPNormal.o \
	$(PLUGIN_OBJ_DIR)/CAOPRadio.o \
	$(PLUGIN_OBJ_DIR)/CAOPSnark.o \
	$(PLUGIN_OBJ_DIR)/CDetour.o \
	$(PLUGIN_OBJ_DIR)/CDetourDis.o \
	$(PLUGIN_OBJ_DIR)/cbase.o \
	$(PLUGIN_OBJ_DIR)/clientcommands.o \
	$(PLUGIN_OBJ_DIR)/CollisionUtils.o \
	$(PLUGIN_OBJ_DIR)/convar_l4d.o \
	$(PLUGIN_OBJ_DIR)/crc32.o \
	$(PLUGIN_OBJ_DIR)/download.o \
	$(PLUGIN_OBJ_DIR)/entitylist.o \
	$(PLUGIN_OBJ_DIR)/entitylist_base.o \
	$(PLUGIN_OBJ_DIR)/funcdetours.o \
	$(PLUGIN_OBJ_DIR)/gamerulesproxy.o \
	$(PLUGIN_OBJ_DIR)/gamestringpool.o \
	$(PLUGIN_OBJ_DIR)/globals.o \
	$(PLUGIN_OBJ_DIR)/igamesystem.o \
	$(PLUGIN_OBJ_DIR)/isopgamesystem.o \
	$(PLUGIN_OBJ_DIR)/l_class_angle.o \
	$(PLUGIN_OBJ_DIR)/l_class_damageinfo.o \
	$(PLUGIN_OBJ_DIR)/l_class_entity.o \
	$(PLUGIN_OBJ_DIR)/l_class_physobj.o \
	$(PLUGIN_OBJ_DIR)/l_class_player.o \
	$(PLUGIN_OBJ_DIR)/l_class_vector.o \
	$(PLUGIN_OBJ_DIR)/lbitlib.o \
	$(PLUGIN_OBJ_DIR)/lconcommand.o \
	$(PLUGIN_OBJ_DIR)/lconstraint.o \
	$(PLUGIN_OBJ_DIR)/lconvar.o \
	$(PLUGIN_OBJ_DIR)/leffects.o \
	$(PLUGIN_OBJ_DIR)/lenginelib.o \
	$(PLUGIN_OBJ_DIR)/lentitycache.o \
	$(PLUGIN_OBJ_DIR)/lents.o \
	$(PLUGIN_OBJ_DIR)/lext.o \
	$(PLUGIN_OBJ_DIR)/lfile.o \
	$(PLUGIN_OBJ_DIR)/lgame.o \
	$(PLUGIN_OBJ_DIR)/lgamerules.o \
	$(PLUGIN_OBJ_DIR)/loslibext.o \
	$(PLUGIN_OBJ_DIR)/lplayer.o \
	$(PLUGIN_OBJ_DIR)/lsopinit.o \
	$(PLUGIN_OBJ_DIR)/lsourceop.o \
	$(PLUGIN_OBJ_DIR)/lstrlibext.o \
	$(PLUGIN_OBJ_DIR)/ltablibext.o \
	$(PLUGIN_OBJ_DIR)/ltempentlib.o \
	$(PLUGIN_OBJ_DIR)/lutil.o \
	$(PLUGIN_OBJ_DIR)/LuaLoader.o \
	$(PLUGIN_OBJ_DIR)/mapcycletracker.o \
	$(PLUGIN_OBJ_DIR)/particles.o \
	$(PLUGIN_OBJ_DIR)/PhysicsShared.o \
	$(PLUGIN_OBJ_DIR)/pingmanager.o \
	$(PLUGIN_OBJ_DIR)/playerdatabase.o \
	$(PLUGIN_OBJ_DIR)/plugin_mm.o \
	$(PLUGIN_OBJ_DIR)/random.o \
	$(PLUGIN_OBJ_DIR)/recipientfilter.o \
	$(PLUGIN_OBJ_DIR)/reservedslots.o \
	$(PLUGIN_OBJ_DIR)/serverplugin_empty.o \
	$(PLUGIN_OBJ_DIR)/sigmgr.o \
	$(PLUGIN_OBJ_DIR)/sopmysql.o \
	$(PLUGIN_OBJ_DIR)/sopsteamid.o \
	$(PLUGIN_OBJ_DIR)/sourceopadmin.o \
	$(PLUGIN_OBJ_DIR)/specialitems.o \
	$(PLUGIN_OBJ_DIR)/studio.o \
	$(PLUGIN_OBJ_DIR)/studio_shared.o \
	$(PLUGIN_OBJ_DIR)/takedamageinfo.o \
	$(PLUGIN_OBJ_DIR)/util.o \
	$(PLUGIN_OBJ_DIR)/utlsymbol_old.o \
	$(PLUGIN_OBJ_DIR)/vfuncs.o \
	$(PLUGIN_OBJ_DIR)/votesystem.o

LUA_OBJS = \
	$(PLUGIN_OBJ_DIR)/lua/src/lapi.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lcode.o \
	$(PLUGIN_OBJ_DIR)/lua/src/ldebug.o \
	$(PLUGIN_OBJ_DIR)/lua/src/ldo.o \
	$(PLUGIN_OBJ_DIR)/lua/src/ldump.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lfunc.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lgc.o \
	$(PLUGIN_OBJ_DIR)/lua/src/linit.o \
	$(PLUGIN_OBJ_DIR)/lua/src/llex.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lmem.o \
	$(PLUGIN_OBJ_DIR)/lua/src/loadlib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lobject.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lopcodes.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lparser.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lstate.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lstring.o \
	$(PLUGIN_OBJ_DIR)/lua/src/ltable.o \
	$(PLUGIN_OBJ_DIR)/lua/src/ltm.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lundump.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lvm.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lzio.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/lauxlib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/lbaselib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/ldblib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/liolib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/lmathlib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/loslib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/lstrlib.o \
	$(PLUGIN_OBJ_DIR)/lua/src/lib/ltablib.o

all: 
	$(MAKE) dirs
	ln -sf $(GAME_DIR)/bin/libvstdlib_srv.so libvstdlib_srv.so
	ln -sf $(GAME_DIR)/bin/libvstdlib_s.so libvstdlib_s.so
	ln -sf $(GAME_DIR)/bin/libtier0_srv.so libtier0_srv.so
	ln -sf ../../lib/linux/tier1_i486.a tier1_srv.a
	ln -sf ../../lib/linux/mathlib_i486.a mathlib_srv.a
#ln -sf $(SOURCE_DIR)/tier1/convar.cpp convar.cpp
	$(MAKE) sourceop.$(SHLIBEXT)

l4d_debug:
	-cp convar_l4d.h ../../public
	$(MAKE) all USER_CFLAGS=-D_L4D_PLUGIN DEBUGBUILD=true
	-rm ../../public/convar_l4d.h

install:
	$(MAKE) all
	$(MAKE) copyfile

install_beta:
	$(MAKE) all
	$(MAKE) copyfile_beta

install_debug:
	$(MAKE) debug
	$(MAKE) copyfile

copyfile:
	-sudo cp sourceop.$(SHLIBEXT) /home/srcds/steampipe/tf/bin/sourceop.$(SHLIBEXT)
	-sudo chown srcds /home/srcds/steampipe/tf/bin/sourceop.$(SHLIBEXT)
	-sudo chgrp srcds /home/srcds/steampipe/tf/bin/sourceop.$(SHLIBEXT)
	-sudo chmod 644 /home/srcds/steampipe/tf/bin/sourceop.$(SHLIBEXT)

copyfile_beta:
	-sudo cp sourceop.$(SHLIBEXT) /home/srcds/tf2betaserver/tf2beta/bin/sourceop.$(SHLIBEXT)
	-sudo chown srcds /home/srcds/tf2betaserver/tf2beta/bin/sourceop.$(SHLIBEXT)
	-sudo chgrp srcds /home/srcds/tf2betaserver/tf2beta/bin/sourceop.$(SHLIBEXT)
	-sudo chmod 644 /home/srcds/tf2betaserver/tf2beta/bin/sourceop.$(SHLIBEXT)

install_l4d:
	$(MAKE) l4d_debug
	-sudo cp sourceop.$(SHLIBEXT) /home/srcds/l4dserver/l4d_demo/bin
	-sudo chown srcds /home/srcds/l4dserver/l4d_demo/bin/sourceop.$(SHLIBEXT)
	-sudo chgrp srcds /home/srcds/l4dserver/l4d_demo/bin/sourceop.$(SHLIBEXT)
	-sudo chmod 644 /home/srcds/l4dserver/l4d_demo/bin/sourceop.$(SHLIBEXT)

dirs:
	-mkdir -p $(BUILD_OBJ_DIR)
	-mkdir -p $(PLUGIN_OBJ_DIR_RELEASE)
	-mkdir -p $(PLUGIN_OBJ_DIR_DEBUG)
	-mkdir -p $(PLUGIN_OBJ_DIR)/lua
	-mkdir -p $(PLUGIN_OBJ_DIR)/lua/src
	-mkdir -p $(PLUGIN_OBJ_DIR)/lua/src/lib
	-mkdir -p $(PLUGIN_OBJ_DIR)/mmsource/sourcehook
	-mkdir -p $(PUBLIC_OBJ_DIR)
	-mkdir -p $(PUBLIC_OBJ_DIR)/../tier1
	$(CHECK_DSP) $(SOURCE_DSP)

sourceop.$(SHLIBEXT): $(LUA_OBJS) $(PLUGIN_OBJS)
	$(CLINK) $(DEBUG) -o $(BUILD_DIR)/$@ $(SHLIBLDFLAGS) $(LUA_OBJS) $(PLUGIN_OBJS) $(CPP_LIB) $(LDFLAGS) $(CPP_LIB) mathlib_srv.a tier1_srv.a

$(PLUGIN_OBJ_DIR)/%.o: $(PLUGIN_SRC_DIR)/%.cpp
	$(DO_CC)

$(PUBLIC_OBJ_DIR)/%.o: $(PUBLIC_SRC_DIR)/%.cpp
	$(DO_CC)

debug:
	$(MAKE) all DEBUGBUILD=true

default: all

clean:
	-rm -rf $(PLUGIN_OBJ_DIR_RELEASE)
	-rm -rf $(PLUGIN_OBJ_DIR_DEBUG)
	-rm -f sourceop.$(SHLIBEXT)
