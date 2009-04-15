#pragma once

class CPatch
{

	
public:
	CPatch();
	~CPatch(void);
	int Parser(CString &pathfile);

	CString m_Author;
	CString m_Date;
	CString m_Subject;
	CString m_PathFile;
	CString m_Body;
};
