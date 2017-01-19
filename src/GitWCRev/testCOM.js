// testCOM.js - javascript file
// test script for the GitWCRev COM/Automation-object

filesystem = new ActiveXObject("Scripting.FileSystemObject");

GitWCRevObject1 = new ActiveXObject("GitWCRev.object");
GitWCRevObject2 = new ActiveXObject("GitWCRev.object");
GitWCRevObject3 = new ActiveXObject("GitWCRev.object");
GitWCRevObject4 = new ActiveXObject("GitWCRev.object");
GitWCRevObject5 = new ActiveXObject("GitWCRev.object");

GitWCRevObject2_1 = new ActiveXObject("GitWCRev.object");
GitWCRevObject2_2 = new ActiveXObject("GitWCRev.object");
GitWCRevObject2_3 = new ActiveXObject("GitWCRev.object");
GitWCRevObject2_4 = new ActiveXObject("GitWCRev.object");
GitWCRevObject2_5 = new ActiveXObject("GitWCRev.object");

GitWCRevObject1.GetWCInfo(filesystem.GetAbsolutePathName("."), 0);
GitWCRevObject2.GetWCInfo(filesystem.GetAbsolutePathName(".."), 0);
GitWCRevObject3.GetWCInfo(filesystem.GetAbsolutePathName("GitWCRev.cpp"), 0);
GitWCRevObject4.GetWCInfo(filesystem.GetAbsolutePathName("..\\.."), 0);

GitWCRevObject2_1.GetWCInfo(filesystem.GetAbsolutePathName("."), 1);
GitWCRevObject2_2.GetWCInfo(filesystem.GetAbsolutePathName(".."), 1);
GitWCRevObject2_3.GetWCInfo(filesystem.GetAbsolutePathName("GitWCRev.cpp"), 1);
GitWCRevObject2_4.GetWCInfo(filesystem.GetAbsolutePathName("..\\.."), 1);

wcInfoString1 = "Revision = " + GitWCRevObject1.Revision + "\nDate = " + GitWCRevObject1.Date + "\nAuthor = " + GitWCRevObject1.Author + "\nHasMods = " + GitWCRevObject1.HasModifications + "\nHasUnversioned = " + GitWCRevObject1.HasUnversioned + "\nIsTagged = " + GitWCRevObject1.IsWcTagged + "\nIsGitItem = " + GitWCRevObject1.IsGitItem+ "\nIsUnborn = " + GitWCRevObject1.IsUnborn + "\nHasSubmodule = " + GitWCRevObject1.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject1.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject1.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject1.IsSubmoduleUp2Date;
wcInfoString2 = "Revision = " + GitWCRevObject2.Revision + "\nDate = " + GitWCRevObject2.Date + "\nAuthor = " + GitWCRevObject2.Author + "\nHasMods = " + GitWCRevObject2.HasModifications + "\nHasUnversioned = " + GitWCRevObject2.HasUnversioned + "\nIsTagged = " + GitWCRevObject2.IsWcTagged + "\nIsGitItem = " + GitWCRevObject2.IsGitItem+ "\nIsUnborn = " + GitWCRevObject2.IsUnborn + "\nHasSubmodule = " + GitWCRevObject2.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject2.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject2.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject2.IsSubmoduleUp2Date;
wcInfoString3 = "Revision = " + GitWCRevObject3.Revision + "\nDate = " + GitWCRevObject3.Date + "\nAuthor = " + GitWCRevObject3.Author + "\nHasMods = " + GitWCRevObject3.HasModifications + "\nHasUnversioned = " + GitWCRevObject3.HasUnversioned + "\nIsTagged = " + GitWCRevObject3.IsWcTagged + "\nIsGitItem = " + GitWCRevObject3.IsGitItem+ "\nIsUnborn = " + GitWCRevObject3.IsUnborn + "\nHasSubmodule = " + GitWCRevObject3.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject3.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject3.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject3.IsSubmoduleUp2Date;
wcInfoString4 = "Revision = " + GitWCRevObject4.Revision + "\nDate = " + GitWCRevObject4.Date + "\nAuthor = " + GitWCRevObject4.Author + "\nHasMods = " + GitWCRevObject4.HasModifications + "\nHasUnversioned = " + GitWCRevObject4.HasUnversioned + "\nIsTagged = " + GitWCRevObject4.IsWcTagged + "\nIsGitItem = " + GitWCRevObject4.IsGitItem+ "\nIsUnborn = " + GitWCRevObject4.IsUnborn + "\nHasSubmodule = " + GitWCRevObject4.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject4.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject4.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject4.IsSubmoduleUp2Date;

WScript.Echo(wcInfoString1 + "\n");
WScript.Echo(wcInfoString2 + "\n");
WScript.Echo(wcInfoString3 + "\n");
WScript.Echo(wcInfoString4 + "\n");

wcInfoString1 = "Revision = " + GitWCRevObject2_1.Revision + "\nDate = " + GitWCRevObject2_1.Date + "\nAuthor = " + GitWCRevObject2_1.Author + "\nHasMods = " + GitWCRevObject2_1.HasModifications + "\nHasUnversioned = " + GitWCRevObject2_1.HasUnversioned + "\nIsTagged = " + GitWCRevObject2_1.IsWcTagged + "\nIsGitItem = " + GitWCRevObject2_1.IsGitItem+ "\nIsUnborn = " + GitWCRevObject2_1.IsUnborn + "\nHasSubmodule = " + GitWCRevObject2_1.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject2_1.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject2_1.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject2_1.IsSubmoduleUp2Date;
wcInfoString2 = "Revision = " + GitWCRevObject2_2.Revision + "\nDate = " + GitWCRevObject2_2.Date + "\nAuthor = " + GitWCRevObject2_2.Author + "\nHasMods = " + GitWCRevObject2_2.HasModifications + "\nHasUnversioned = " + GitWCRevObject2_2.HasUnversioned + "\nIsTagged = " + GitWCRevObject2_2.IsWcTagged + "\nIsGitItem = " + GitWCRevObject2_2.IsGitItem+ "\nIsUnborn = " + GitWCRevObject2_2.IsUnborn + "\nHasSubmodule = " + GitWCRevObject2_2.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject2_2.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject2_2.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject2_2.IsSubmoduleUp2Date;
wcInfoString3 = "Revision = " + GitWCRevObject2_3.Revision + "\nDate = " + GitWCRevObject2_3.Date + "\nAuthor = " + GitWCRevObject2_3.Author + "\nHasMods = " + GitWCRevObject2_3.HasModifications + "\nHasUnversioned = " + GitWCRevObject2_3.HasUnversioned + "\nIsTagged = " + GitWCRevObject2_3.IsWcTagged + "\nIsGitItem = " + GitWCRevObject2_3.IsGitItem+ "\nIsUnborn = " + GitWCRevObject2_3.IsUnborn + "\nHasSubmodule = " + GitWCRevObject2_3.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject2_3.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject2_3.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject2_3.IsSubmoduleUp2Date;
wcInfoString4 = "Revision = " + GitWCRevObject2_4.Revision + "\nDate = " + GitWCRevObject2_4.Date + "\nAuthor = " + GitWCRevObject2_4.Author + "\nHasMods = " + GitWCRevObject2_4.HasModifications + "\nHasUnversioned = " + GitWCRevObject2_4.HasUnversioned + "\nIsTagged = " + GitWCRevObject2_4.IsWcTagged + "\nIsGitItem = " + GitWCRevObject2_4.IsGitItem+ "\nIsUnborn = " + GitWCRevObject2_4.IsUnborn + "\nHasSubmodule = " + GitWCRevObject2_4.HasSubmodule + "\nHasSubmoduleModifications = " + GitWCRevObject2_4.HasSubmoduleModifications + "\nHasSubmoduleUnversioned = " + GitWCRevObject2_4.HasSubmoduleUnversioned + "\nIsSubmoduleUp2Date = " + GitWCRevObject2_4.IsSubmoduleUp2Date;

WScript.Echo(wcInfoString1 + "\n");
WScript.Echo(wcInfoString2 + "\n");
WScript.Echo(wcInfoString3 + "\n");
WScript.Echo(wcInfoString4 + "\n");
