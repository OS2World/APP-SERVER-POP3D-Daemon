#
# Makefile for POP3 daemon
#
# Bob Eager   June 2002
#
# Product names
#
PRODUCT		= pop3d
UPMDLL		= auth_upm
ALTDLL		= auth_alt
TEST		= test
#
# Target directories etc.
#
TARGET		= f:\binp
TARGETL		= f:\otherdll
TARGETN		= pop3auth.dll
#
# Compiler setup
#
CC		= icc
DEFS		= -DTCPV40HDRS
!IFDEF DEBUG
DBUG		= -DDEBUG
!ELSE
DBUG		=
!ENDIF
!IFDEF	PROD
CFLAGS		= $(DEFS) $(DBUG) -Fi -G4 -Gm -Gn -O -Q -Se -Si -Tl30
CFLAGSL		= $(DEFS) $(DBUG) -Fi -G4 -Ge- -Gm -O -Q -Se -Si -Tl30
CFLAGST		= $(DEFS) $(DBUG) -Fi -G4 -Gm -O -Q -Se -Si -Tl30
CLIB		= cppom30o.lib
!ELSE
CFLAGS		= $(DEFS) $(DBUG) -Fi -G4 -Gm -Gn -Q -Se -Si -Ti -Tl30 -Tm -Tx
CFLAGSL		= $(DEFS) $(DBUG) -Fi -G4 -Ge- -Gm -Q -Se -Si -Ti -Tl30 -Tm -Tx
CFLAGST		= $(DEFS) $(DBUG) -Fi -G4 -Gm -Q -Se -Si -Ti -Tl30 -Tm -Tx
CLIB		= cppom30.lib
!ENDIF
#
# Names of library files
#
UPM		= upm32.lib
NETLIB		= ..\netlib\netlib.lib
LIBS		= so32dll.lib tcp32dll.lib $(UPM) $(CLIB) \
		  $(NETLIB) os2386.lib
UPMLIB		= $(UPMDLL).lib
ALTLIB		= $(ALTDLL).lib
#
# Names of object files
#
OBJ		= pop3d.obj server.obj lock.obj maildrop.obj \
		  netio.obj log.obj
UPMOBJ		= $(UPMDLL).obj
ALTOBJ		= $(ALTDLL).obj
#
# Other files
#
DEF		= $(PRODUCT).def
UPMDEF		= $(UPMDLL).def
ALTDEF		= $(ALTDLL).def
LNK		= $(PRODUCT).lnk
UPMLNK		= $(UPMDLL).lnk
ALTLNK		= $(ALTDLL).lnk
README		= readme.txt
OTHER		= todo.txt
#
# Final executable file and DLLs
#
EXE		= $(PRODUCT).exe
UPMDLL_DLL	= $(UPMDLL).dll
ALTDLL_DLL	= $(ALTDLL).dll
#
# Distribution
#
DIST		= dist.zip
NETLIBDLL	= ..\netlib\netlib.dll
#
# Archive
#
ARCH		= arch.zip
#
#-----------------------------------------------------------------------------
#
$(EXE):		$(OBJ) $(LNK) $(DEF) $(UPMDLL_DLL) $(ALTDLL_DLL) \
		$(UPMLIB) $(ALTLIB)
!IFDEF	PROD
		ilink /nodefaultlibrarysearch /nologo /exepack:2 @$(LNK)
		lxlite $(EXE)
		eautil $(EXE) nul /s
!ELSE
		ilink /nodefaultlibrarysearch /debug /nobrowse /nologo @$(LNK)
!ENDIF
#
$(UPMDLL_DLL):	$(UPMOBJ) $(UPMLNK) $(UPMDEF)
		ilink /nodefaultlibrarysearch /nologo /exepack:2 @$(UPMLNK)
!IFDEF	PROD
		lxlite $(UPMDLL_DLL)
		eautil $(UPMDLL_DLL) nul /s
!ENDIF
#
$(UPMLIB):	$(UPMDEF)
		implib /nologo /noignorecase $(UPMLIB) $(UPMDEF)
#
$(ALTDLL_DLL):	$(ALTOBJ) $(ALTLNK) $(ALTDEF)
		ilink /nodefaultlibrarysearch /nologo /exepack:2 @$(ALTLNK)
!IFDEF	PROD
		lxlite $(ALTDLL_DLL)
		eautil $(ALTDLL_DLL) nul /s
!ENDIF
#
$(ALTLIB):	$(ALTDEF)
		implib /nologo /noignorecase $(ALTLIB) $(ALTDEF)
#
# Object files
#
pop3d.obj:	pop3d.c pop3d.h log.h
#
server.obj:	server.c pop3d.h cmds.h lock.h auth.h maildrop.h netio.h log.h
#
!IFDEF DEBUG
maildrop.obj:	maildrop.c maildrop.h log.h
!ELSE
maildrop.obj:	maildrop.c maildrop.h
!ENDIF
#
!IFDEF DEBUG
lock.obj:	lock.c lock.h log.h
!ELSE
lock.obj:	lock.c lock.h
!ENDIF
#
netio.obj:	netio.c netio.h
#
log.obj:	log.c log.h
#
auth_upm.obj:	auth_upm.c auth.h
		$(CC) $(CFLAGSL) /c auth_upm.c
#
auth_alt.obj:	auth_alt.c auth.h
		$(CC) $(CFLAGSL) /c auth_alt.c
#
# Linker response files. Rebuild if makefile changes
#
$(LNK):		makefile
		@if exist $(LNK) erase $(LNK)
		@echo /map:$(PRODUCT) >> $(LNK)
		@echo /out:$(PRODUCT) >> $(LNK)
		@echo $(OBJ) >> $(LNK)
		@echo $(LIBS) >> $(LNK)
# Any auth library would suffice on the next line...
		@echo $(UPMLIB) >> $(LNK)
		@echo $(DEF) >> $(LNK)
#
$(UPMLNK):	makefile
		@if exist $(UPMLNK) erase $(UPMLNK)
		@echo /map:$(UPMDLL) >> $(UPMLNK)
		@echo /out:$(UPMDLL) >> $(UPMLNK)
		@echo $(UPMOBJ) >> $(UPMLNK)
		@echo cppom30o.lib os2386.lib >> $(UPMLNK)
		@echo $(NETLIB) >> $(UPMLNK)
		@echo $(UPM) >> $(UPMLNK)
		@echo $(UPMDEF) >> $(UPMLNK)
#
$(ALTLNK):	makefile
		@if exist $(ALTLNK) erase $(ALTLNK)
		@echo /map:$(ALTDLL) >> $(ALTLNK)
		@echo /out:$(ALTDLL) >> $(ALTLNK)
		@echo $(ALTOBJ) >> $(ALTLNK)
		@echo cppom30o.lib os2386.lib >> $(ALTLNK)
		@echo $(NETLIB) >> $(ALTLNK)
		@echo $(ALTDEF) >> $(ALTLNK)
#
test:		$(TEST).exe
#
$(TEST).exe:	$(TEST).obj log.obj
		ilink /map /debug /nobrowse /nologo /pmtype:vio $(TEST).obj log.obj $(UPMLIB)
#
$(TEST).obj:	$(TEST).c auth.h log.h $(UPMDLL_DLL) $(ALTDLL_DLL)
		$(CC) $(CFLAGST) /c $*.c
#
clean:		
		-erase $(OBJ) $(LNK) $(PRODUCT).map csetc.pch
		-erase $(UPMOBJ) $(ALTOBJ) $(UPMLIB) $(ALTLIB)
		-erase $(UPMLNK) $(ALTLNK) $(UPMDLL).map $(ALTDLL).map
		-erase $(TEST).exe $(TEST).obj $(TEST).map
#
install:	$(EXE)
		@copy $(EXE) $(TARGET) > nul
#
install_upm:	$(UPMDLL_DLL)
		@copy $(UPMDLL_DLL) $(TARGETL)\$(TARGETN) > nul
#
install_alt:	$(ALTDLL_DLL)
		@copy $(ALTDLL_DLL) $(TARGETL)\$(TARGETN) > nul
#
dist:		$(EXE) $(NETLIBDLL) $(UPMDLL_DLL) $(ALTDLL_DLL) $(README)
		zip -9 -j $(DIST) $**
#
arch:		$(EXE) $(UPMDLL_DLL) $(ALTDLL_DLL) $(README) $(OTHER) \
		*.def *.c *.h makefile
		zip -9 -j $(ARCH) $**
#
# End of makefile for POP3 daemon
#
