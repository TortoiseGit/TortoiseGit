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
	
$(outdir)/TortoiseProc2052.dll:Tortoise_zh_CN.po
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseProcLang.dll $@ $?
$(outdir)/TortoiseMerge2052.dll:Tortoise_zh_CN.po
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseMergeLang.dll $@ $?
$(outdir)/TortoiseIdiff2052.dll:Tortoise_zh_CN.po
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseIdiffLang.dll $@ $?
$(outdir)/TortoiseGitBlame2052.dll:Tortoise_zh_CN.po
  $(RESTEXT) $(APPLY) $(outdir)\TortoiseGitBlameLang.dll $@ $?
