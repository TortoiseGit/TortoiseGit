###########################
#  
# Build Tortoise Language 
#
###########################

RESTEXT		=..\bin\$(ConfigurationName)\bin\ResText.exe
APPLY		=apply
LANGDLLS	=$(outdir)/TortoiseProcLang2052.dll
all:$(LANGDLLS)
	
$(outdir)/TortoiseProcLang2052.dll:Tortoise_zh_CN.po
	$(RESTEXT) $(APPLY) $(outdir)\TortoiseProcLang.dll $(outdir)\TortoiseProcLang2052.dll Tortoise_zh_CN.po

