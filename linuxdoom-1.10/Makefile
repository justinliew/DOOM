################################################################
#
# $Id:$
#
# $Log:$
#
#CC=  gcc  # gcc or g++
CC=/opt/wasi-sdk-20.0/bin/clang --sysroot /opt/wasi-sdk-20.0/share/wasi-sysroot

#CFLAGS=-g -Wall -Wno-unused-const-variable -Wno-implicit-function-declaration -DHEADLESS -DWASISDK 
CFLAGS=-g -Wall -Wno-absolute-value -Wno-logical-not-parentheses -Wno-sizeof-pointer-memaccess -Wno-uninitialized -Wno-pointer-sign -Wno-incompatible-pointer-types -Wno-unsequenced -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-unused-variable -Wno-implicit-function-declaration -DHEADLESS -D_WASI_EMULATED_PROCESS_CLOCKS -DWASISDK -DXQD -DQUIET_XQD # -DUSEASM 
#CFLAGS=-g -Wall -Wno-unused-const-variable -Wno-implicit-function-declaration -DHEADLESS -DWASISDK -DXQDMP -DXQD -Wl,--allow-undefined # -DUSEASM 
LDFLAGS=-Wl,--allow-undefined -lwasi-emulated-process-clocks
LIBS=-lm 

# subdirectory for objects
O=linux

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_system.o		\
		$(O)/i_net.o			\
		$(O)/tables.o			\
		$(O)/f_finale.o		\
		$(O)/f_wipe.o 		\
		$(O)/d_main.o			\
		$(O)/memorywad.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o		\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  		\
		$(O)/m_bbox.o			\
		$(O)/m_fixed.o		\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o		\
		$(O)/m_random.o		\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o		\
		$(O)/p_doors.o		\
		$(O)/p_enemy.o		\
		$(O)/p_floor.o		\
		$(O)/p_inter.o		\
		$(O)/p_lights.o		\
		$(O)/p_map.o			\
		$(O)/p_maputl.o		\
		$(O)/p_plats.o		\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o		\
		$(O)/p_sight.o		\
		$(O)/p_spec.o			\
		$(O)/p_switch.o		\
		$(O)/p_mobj.o			\
		$(O)/p_telept.o		\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o		\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o		\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o		\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o		\
		$(O)/v_video.o		\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o		\
		$(O)/hu_stuff.o		\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o		\
		$(O)/z_zone.o			\
		$(O)/info.o				\
		$(O)/sounds.o			\
		$(O)/miniz.o			\
		$(O)/base64.o			\
		$(O)/x_cache.o			\

all:	 $(O)/linuxxdoom

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*.o
	rm linux/linuxxdoom

$(O)/linuxxdoom:	$(OBJS) $(O)/i_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_main.o \
	-o $(O)/linuxxdoom $(LIBS)

$(O)/%.o:	%.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################