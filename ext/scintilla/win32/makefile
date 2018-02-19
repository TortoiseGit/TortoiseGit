# Make file for Scintilla on Windows
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes the Mingw-w64 version of GCC 7.1+ is used and changes will
# be needed to use other compilers.

ifdef CLANG
# Common location for 32-bit clang on Windows
CXX = "C:\Program Files (x86)\LLVM\bin\clang++"
CXXSTD = c++14
else
# MinGW GCC
LDMINGW = -Wl,--enable-runtime-pseudo-reloc-v2 -Wl,--add-stdcall-alias
LIBSMINGW = -lstdc++
STRIPOPTION = -s
CXXSTD = gnu++17
endif

.SUFFIXES: .cxx
ifeq ($(OS),Windows_NT)
DEL = del /q
else
DEL = rm -f
endif
RANLIB ?= ranlib
WINDRES ?= windres

COMPONENT = ../bin/Scintilla.dll
LEXCOMPONENT = ../bin/SciLexer.dll
LEXLIB = Lexers.a

vpath %.h ../src ../include ../lexlib
vpath %.cxx ../src ../lexlib ../lexers

LDFLAGS=-shared -static -mwindows $(LDMINGW)
LIBS=-lgdi32 -luser32 -limm32 -lole32 -luuid -loleaut32 -lmsimg32 $(LIBSMINGW)
# Add -MMD to get dependencies
INCLUDEDIRS=-I ../include -I ../src -I../lexlib

# Check if Direct2D headers are available by trying to compile a file that includes them.
# Most distributions of MinGW32 do not include Direct2D support but MinGW64 does.
ifneq "$(shell $(CXX) -c CheckD2D.cxx 2>&1)" ""
D2DFLAGS=-DDISABLE_D2D
endif

ifdef ENABLE_BIDIRECTIONAL
BIDIFLAGS=-DENABLE_BIDIRECTIONAL
endif

CXXBASEFLAGS=--std=$(CXXSTD) -Wall -pedantic $(INCLUDEDIRS) $(D2DFLAGS) $(BIDIFLAGS) -D_CRT_SECURE_NO_DEPRECATE=1

ifdef NO_CXX11_REGEX
REFLAGS=-DNO_CXX11_REGEX
endif

ifdef DEBUG
CXXFLAGS=-DDEBUG -g $(CXXBASEFLAGS)
else
CXXFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS)
STRIPFLAG=$(STRIPOPTION)
endif

.cxx.o:
	$(CXX) $(CXXFLAGS) $(REFLAGS) -c $<

ALL:	$(COMPONENT) $(LEXCOMPONENT) $(LEXLIB) ScintillaWinS.o

clean:
	$(DEL) *.exe *.o *.obj *.dll *.res *.map *.plist

analyze:
	$(CXX) --analyze $(D2DFLAGS) $(CXXFLAGS) *.cxx ../src/*.cxx ../lexlib/*.cxx ../lexers/*.cxx

deps:
	$(CXX) -MM $(CXXFLAGS) *.cxx ../src/*.cxx ../lexlib/*.cxx ../lexers/*.cxx >deps.mak

LEXOBJS:=$(addsuffix .o,$(basename $(sort $(notdir $(wildcard ../lexers/Lex*.cxx)))))


BASEOBJS = \
	AutoComplete.o \
	CallTip.o \
	CaseConvert.o \
	CaseFolder.o \
	CellBuffer.o \
	CharacterCategory.o \
	CharacterSet.o \
	CharClassify.o \
	ContractionState.o \
	DBCS.o \
	Decoration.o \
	Document.o \
	EditModel.o \
	Editor.o \
	EditView.o \
	KeyMap.o \
	Indicator.o \
	LineMarker.o \
	MarginView.o \
	PerLine.o \
	PlatWin.o \
	PositionCache.o \
	PropSetSimple.o \
	RESearch.o \
	RunStyles.o \
	ScintRes.o \
	Selection.o \
	Style.o \
	UniConversion.o \
	ViewStyle.o \
	XPM.o \
	HanjaDic.o

SOBJS = ScintillaWin.o ScintillaBase.o $(BASEOBJS)

$(COMPONENT): $(SOBJS) Scintilla.def
	$(CXX) $(LDFLAGS) -o $@ $(STRIPFLAG) $(SOBJS) $(CXXFLAGS) $(LIBS)

LOBJS = \
	Accessor.o \
	Catalogue.o \
	ExternalLexer.o \
	DefaultLexer.o \
	LexerBase.o \
	LexerModule.o \
	LexerSimple.o \
	ScintillaWinL.o \
	ScintillaBaseL.o \
	StyleContext.o \
	WordList.o \
	$(BASEOBJS) \
	$(LEXOBJS)
$(LEXCOMPONENT): $(LOBJS) Scintilla.def
	$(CXX) $(LDFLAGS) -o $@ $(STRIPFLAG) $(LOBJS) $(CXXFLAGS) $(LIBS)

$(LEXLIB): $(LEXOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

# Automatically generate dependencies for most files with "make deps"
include deps.mak

# These dependencies are maintained by hand as they do not use the default output name

ScintillaBaseL.o: ScintillaBase.cxx Platform.h \
 ILexer.h Scintilla.h SciLexer.h PropSetSimple.h \
 SplitVector.h Partitioning.h RunStyles.h \
 ContractionState.h CellBuffer.h CallTip.h \
 KeyMap.h Indicator.h XPM.h LineMarker.h \
 Style.h ViewStyle.h AutoComplete.h \
 CharClassify.h Decoration.h Document.h \
 Selection.h PositionCache.h EditModel.h Editor.h EditView.h \
 ScintillaBase.h LexAccessor.h Accessor.h \
 LexerModule.h Catalogue.h CaseFolder.h

ScintillaWinL.o: ScintillaWin.cxx Platform.h \
 ILexer.h Scintilla.h SplitVector.h \
 Partitioning.h RunStyles.h ContractionState.h \
 CellBuffer.h CallTip.h KeyMap.h Indicator.h \
 XPM.h LineMarker.h Style.h AutoComplete.h \
 ViewStyle.h CharClassify.h Decoration.h \
 Document.h Selection.h PositionCache.h \
 EditModel.h Editor.h EditView.h ScintillaBase.h UniConversion.h \
 LexAccessor.h Accessor.h \
 LexerModule.h Catalogue.h CaseConvert.h \
 CaseFolder.h

ScintillaWinS.o: ScintillaWin.cxx Platform.h \
 ILexer.h Scintilla.h SplitVector.h \
 Partitioning.h RunStyles.h ContractionState.h \
 CellBuffer.h CallTip.h KeyMap.h Indicator.h \
 XPM.h LineMarker.h Style.h AutoComplete.h \
 ViewStyle.h CharClassify.h Decoration.h \
 Document.h Selection.h PositionCache.h \
 EditModel.h Editor.h EditView.h ScintillaBase.h UniConversion.h \
 CaseConvert.h CaseFolder.h

ScintillaBaseL.o:
	$(CXX) $(CXXFLAGS) -D SCI_LEXER -c $< -o $@

ScintillaWinS.o:
	$(CXX) $(CXXFLAGS) -D STATIC_BUILD -c $< -o $@

ScintillaWinL.o:
	$(CXX) $(CXXFLAGS) -D SCI_LEXER -c $< -o $@

ScintRes.o:	ScintRes.rc
	$(WINDRES) ScintRes.rc $@

