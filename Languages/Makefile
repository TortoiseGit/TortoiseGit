###########################
#  
# Build Tortoise Language 
#
###########################

RESTEXT		= ..\bin\$(ConfigurationName)\bin\ResText.exe
APPLY		= apply
LANGDLLS	=	$(outdir)/TortoiseProc2052.dll \
				$(outdir)/TortoiseMerge2052.dll \
				$(outdir)/TortoiseIdiff2052.dll \
				$(outdir)/TortoiseGitBlame2052.dll \
				$(outdir)/TortoiseProc1031.dll \
				$(outdir)/TortoiseMerge1031.dll \
				$(outdir)/TortoiseIdiff1031.dll \
				$(outdir)/TortoiseGitBlame1031.dll \

all:$(LANGDLLS)

$(outdir)/TortoiseProc2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseProcLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseProcLang.dll $@ Tortoise_zh_CN.po
$(outdir)/TortoiseMerge2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseMergeLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseMergeLang.dll $@ Tortoise_zh_CN.po
$(outdir)/TortoiseIdiff2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseIdiffLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseIdiffLang.dll $@ Tortoise_zh_CN.po
$(outdir)/TortoiseGitBlame2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseGitBlameLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseGitBlameLang.dll $@ Tortoise_zh_CN.po

$(outdir)/TortoiseProc1031.dll:Tortoise_de.po $(outdir)\TortoiseProcLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseProcLang.dll $@ Tortoise_de.po
$(outdir)/TortoiseMerge1031.dll:Tortoise_de.po $(outdir)\TortoiseMergeLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseMergeLang.dll $@ Tortoise_de.po
$(outdir)/TortoiseIdiff1031.dll:Tortoise_de.po $(outdir)\TortoiseIdiffLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseIdiffLang.dll $@ Tortoise_de.po
$(outdir)/TortoiseGitBlame1031.dll:Tortoise_de.po $(outdir)\TortoiseGitBlameLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseGitBlameLang.dll $@ Tortoise_de.po
