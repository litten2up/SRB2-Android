#!/bin/bash
PREFIX="/opt/devkitpro/devkitARM/bin/arm-none-eabi" CC="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc -specs=3dsx.specs" CPPFLAGS="-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -UUNIXCOMMON -ULUA_USE_POSIX -DLOGMESSAGES -DHAVE_DOSSTR_FUNCS -D__3DS__ -DARM11 -DHAVE_SDL -UALLOW_RESETDATA -DNO_IPV6 -I/opt/devkitpro/libctru/include" PKG_CONFIG="/opt/devkitpro/portlibs/3ds/bin/arm-none-eabi-pkg-config" SDL_CONFIG="/opt/devkitpro/portlibs/3ds/bin/sdl2-config" PNG_CONFIG="/opt/devkitpro/portlibs/3ds/bin/libpng-config" LDFLAGS="-specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -L/opt/devkitpro/libctru/lib -lmodplug -lgme -lxmp -lmpg123 -lFLAC -lopusfile -logg -lopus -lctru -lm -lstdc++" make SDL=1 NONX86=1 -C src/ NOOPENMPT=1 NOHS=1 NOHW=1 NOGME=1 NOCURL=1 UNIX=1 NOSDLMAIN=1 ECHO=1 NOIPV6=1 HAVE_MIXERX=1 -j4 clean |&  tee srb2build.log
PREFIX="/opt/devkitpro/devkitARM/bin/arm-none-eabi" CC="/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc -specs=3dsx.specs" CPPFLAGS="-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -UUNIXCOMMON -ULUA_USE_POSIX -DLOGMESSAGES -DHAVE_DOSSTR_FUNCS -D__3DS__ -DARM11 -DHAVE_SDL -UALLOW_RESETDATA -DNO_IPV6 -I/opt/devkitpro/libctru/include" PKG_CONFIG="/opt/devkitpro/portlibs/3ds/bin/arm-none-eabi-pkg-config" SDL_CONFIG="/opt/devkitpro/portlibs/3ds/bin/sdl2-config" PNG_CONFIG="/opt/devkitpro/portlibs/3ds/bin/libpng-config" LDFLAGS="-specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -L/opt/devkitpro/libctru/lib -lmodplug -lgme -lxmp -lmpg123 -lFLAC -lopusfile -logg -lopus -lctru -lm -lstdc++" make SDL=1 NONX86=1 -C src/ NOOPENMPT=1 NOHS=1 NOHW=1 NOGME=1 NOCURL=1 UNIX=1 NOSDLMAIN=1 ECHO=1 NOIPV6=1 HAVE_MIXERX=1 -j4 |&  tee srb2build.log
echo "Building cia"
../Project_CTR/makerom/bin/makerom -f cia -target t -exefslogo -o bin/lsdl2srb2.cia -elf bin/lsdl2srb2.debug -rsf meta/app.rsf -banner meta/banner.bin -icon bin/srb2_3ds.smdh -logo meta/logo.bcma.lz
echo "Built cia"
