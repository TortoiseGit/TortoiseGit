// testCOMMemory.js - javascript file
// test script for the GitWCRev COM/Automation-object

var i, git;

git = new ActiveXObject("GitWCRev.object");

for (i = 0; i < 10000; ++i) {
	git.GetWCInfo("D:\\TortoiseGit\\build.txt", false);
}
