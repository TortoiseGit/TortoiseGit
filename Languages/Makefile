###########################
#  
# Build Tortoise Language 
#
###########################

RESTEXT		=..\bin\$(ConfigurationName)\bin\ResText.exe
APPLY		=apply
LANGDLLS	=$(outdir)/TortoiseProc2052.dll   \
           $(outdir)/TortoiseMerge2052.dll  \
           $(outdir)/TortoiseIdiff2052.dll  \
           $(outdir)/TortoiseGitBlame2052.dll  \
           
all:$(LANGDLLS)
	
$(outdir)/TortoiseProc2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseProcLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseProcLang.dll $@ Tortoise_zh_CN.po
$(outdir)/TortoiseMerge2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseMergeLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseMergeLang.dll $@ Tortoise_zh_CN.po
$(outdir)/TortoiseIdiff2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseIdiffLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseIdiffLang.dll $@ Tortoise_zh_CN.po
$(outdir)/TortoiseGitBlame2052.dll:Tortoise_zh_CN.po $(outdir)\TortoiseGitBlameLang.dll
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseGitBlameLang.dll $@ Tortoise_zh_CN.po        