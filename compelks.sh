set -e

CCOPTSS="-os             -bt=none -0 -zq -s -mm -wx -zastd=c99 -zls"
CCOPTST="-oaxet -oh -ol+ -bt=none -0 -zq -s -mm -wx -zastd=c99 -zls"

nasm i_vmodya.asm -f obj -DCPU=i8088
nasm m_fixed.asm  -f obj

wcc $CCOPTSS am_map.c   -fo=am_map.obj
wcc $CCOPTSS d_items.c  -fo=d_items.obj
wcc $CCOPTSS d_main.c   -fo=d_main.obj
wcc $CCOPTSS f_finale.c -fo=f_finale.obj
wcc $CCOPTSS g_game.c   -fo=g_game.obj
wcc $CCOPTSS hu_stuff.c -fo=hu_stuff.obj
wcc $CCOPTSS i_audio.c  -fo=i_audio.obj
wcc $CCOPTSS i_main.c   -fo=i_main.obj
wcc $CCOPTSS i_elks.c   -fo=i_elks.obj
wcc $CCOPTSS info.c     -fo=info.obj
wcc $CCOPTSS m_cheat.c  -fo=m_cheat.obj
wcc $CCOPTSS m_menu.c   -fo=m_menu.obj
wcc $CCOPTSS m_random.c -fo=m_random.obj
wcc $CCOPTSS p_doors.c  -fo=p_doors.obj
wcc $CCOPTSS p_enemy.c  -fo=p_enemy.obj
wcc $CCOPTSS p_floor.c  -fo=p_floor.obj
wcc $CCOPTSS p_inter.c  -fo=p_inter.obj
wcc $CCOPTSS p_lights.c -fo=p_lights.obj
wcc $CCOPTSS p_plats.c  -fo=p_plats.obj
wcc $CCOPTSS p_pspr.c   -fo=p_pspr.obj
wcc $CCOPTSS p_setup.c  -fo=p_setup.obj
wcc $CCOPTSS p_spec.c   -fo=p_spec.obj
wcc $CCOPTSS p_switch.c -fo=p_switch.obj
wcc $CCOPTSS p_telept.c -fo=p_telept.obj
wcc $CCOPTSS p_tick.c   -fo=p_tick.obj
wcc $CCOPTSS p_user.c   -fo=p_user.obj
wcc $CCOPTSS r_data.c   -fo=r_data.obj
wcc $CCOPTSS r_sky.c    -fo=r_sky.obj
wcc $CCOPTSS r_things.c -fo=r_things.obj
wcc $CCOPTSS s_sound.c  -fo=s_sound.obj
wcc $CCOPTSS sounds.c   -fo=sounds.obj
wcc $CCOPTSS st_stuff.c -fo=st_stuff.obj
wcc $CCOPTSS v_video.c  -fo=v_video.obj
wcc $CCOPTSS wi_stuff.c -fo=wi_stuff.obj
wcc $CCOPTSS z_bmallo.c -fo=z_bmallo.obj

wcc $CCOPTST i_vmodey.c -fo=i_vmodey.obj
wcc $CCOPTST p_map.c    -fo=p_map.obj
wcc $CCOPTST p_maputl.c -fo=p_maputl.obj
wcc $CCOPTST p_mobj.c   -fo=p_mobj.obj
wcc $CCOPTST p_sight.c  -fo=p_sight.obj
wcc $CCOPTST r_draw.c   -fo=r_draw.obj
wcc $CCOPTST r_plane.c  -fo=r_plane.obj
wcc $CCOPTST tables.c   -fo=tables.obj
wcc $CCOPTST w_wad.c    -fo=w_wad.obj
wcc $CCOPTST z_zone.c   -fo=z_zone.obj

owcc -bos2 -s -Wl,option -Wl,start=_start -Wl,alias -Wl,source=_source -Wl,alias -Wl,nearcolormap=_nearcolormap -Wl,alias -Wl,dest=_dest -Wl,alias -Wl,R_DrawColumn2_=R_DrawColumn2 -Wl,alias -Wl,FixedReciprocal_=FixedReciprocal -Wl,alias -Wl,FixedReciprocalBig_=FixedReciprocalBig -Wl,alias -Wl,FixedReciprocalSmall_=FixedReciprocalSmall -Wl,option -Wl,dosseg -Wl,option -Wl,nodefaultlibs -Wl,option -Wl,stack=0x1000 -Wl,option -Wl,heapsize=0x1000 -Wl,library -Wl,$LIBC -o elksdoom.os2 am_map.obj d_items.obj d_main.obj f_finale.obj g_game.obj hu_stuff.obj i_audio.obj i_main.obj i_elks.obj i_vmodey.obj i_vmodya.obj info.obj m_cheat.obj m_fixed.obj m_menu.obj m_random.obj p_doors.obj p_enemy.obj p_floor.obj p_inter.obj p_lights.obj p_map.obj p_maputl.obj p_mobj.obj p_plats.obj p_pspr.obj p_setup.obj p_sight.obj p_spec.obj p_switch.obj p_telept.obj p_tick.obj p_user.obj r_data.obj r_draw.obj r_plane.obj r_sky.obj r_things.obj s_sound.obj sounds.obj st_stuff.obj tables.obj v_video.obj w_wad.obj wi_stuff.obj z_bmallo.obj z_zone.obj

rm *.obj
rm *.err
