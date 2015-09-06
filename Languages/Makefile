#############################
#                           #
# Build a Tortoise Language #
#                           #
#############################

!ifndef outdir
!ERROR The outdir variable has to be set!
!endif
!ifndef ISO
!ERROR The ISO variable has to be set!
!endif
!ifndef LANGID
!ERROR The LANGID variable has to be set!
!endif

RESTEXT		= "$(outdir)\ResText.exe"
APPLY		= apply
LANGDLLS	=	"$(outdir)\TortoiseProc$(LANGID).dll" \
				"$(outdir)\TortoiseMerge$(LANGID).dll" \
				"$(outdir)\TortoiseIdiff$(LANGID).dll" \
				"$(outdir)\TortoiseGitUDiff$(LANGID).dll" \
				"$(outdir)\TortoiseGitBlame$(LANGID).dll"

all: $(LANGDLLS)

clean:
	del $(LANGDLLS)

"$(outdir)\TortoiseProc$(LANGID).dll": Tortoise_$(ISO).po "$(outdir)\TortoiseProcLang.dll"
  $(RESTEXT) $(APPLY) "$(outdir)\TortoiseProcLang.dll" $@ Tortoise_$(ISO).po

"$(outdir)\TortoiseMerge$(LANGID).dll": Tortoise_$(ISO).po "$(outdir)\TortoiseMergeLang.dll"
  $(RESTEXT) $(APPLY) "$(outdir)\TortoiseMergeLang.dll" $@ Tortoise_$(ISO).po

"$(outdir)\TortoiseIDiff$(LANGID).dll": Tortoise_$(ISO).po "$(outdir)\TortoiseIDiffLang.dll"
  $(RESTEXT) $(APPLY) "$(outdir)\TortoiseIdiffLang.dll" $@ Tortoise_$(ISO).po

"$(outdir)\TortoiseGitUDiff$(LANGID).dll": Tortoise_$(ISO).po "$(outdir)\TortoiseGitUDiffLang.dll"
  $(RESTEXT) $(APPLY) "$(outdir)\TortoiseGitUDiffLang.dll" $@ Tortoise_$(ISO).po

"$(outdir)\TortoiseGitBlame$(LANGID).dll": Tortoise_$(ISO).po "$(outdir)\TortoiseGitBlameLang.dll"
  $(RESTEXT) $(APPLY) "$(outdir)\TortoiseGitBlameLang.dll" $@ Tortoise_$(ISO).po
