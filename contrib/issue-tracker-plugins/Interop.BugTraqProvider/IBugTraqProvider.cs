using System;
using System.Runtime.InteropServices;

namespace Interop.BugTraqProvider
{
    [ComVisible(true), InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("298B927C-7220-423C-B7B4-6E241F00CD93")]
    public interface IBugTraqProvider
    {
        [return: MarshalAs(UnmanagedType.VariantBool)]
        bool ValidateParameters(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetLinkText(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetCommitMessage(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters,
            [MarshalAs(UnmanagedType.BStr)] string commonRoot,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType=VarEnum.VT_BSTR)] string[] pathList,
            [MarshalAs(UnmanagedType.BStr)] string originalMessage);
    }

	[ComVisible( true ), InterfaceType( ComInterfaceType.InterfaceIsIUnknown ), Guid( "C5C85E31-2F9B-4916-A7BA-8E27D481EE83" )]
	public interface IBugTraqProvider2
	{
		[return: MarshalAs( UnmanagedType.VariantBool )]
		bool ValidateParameters( IntPtr hParentWnd,
			[MarshalAs( UnmanagedType.BStr )] string parameters );

		[return: MarshalAs( UnmanagedType.BStr )]
		string GetLinkText( IntPtr hParentWnd,
			[MarshalAs( UnmanagedType.BStr )] string parameters );

		[return: MarshalAs( UnmanagedType.BStr )]
		string GetCommitMessage( IntPtr hParentWnd,
			[MarshalAs( UnmanagedType.BStr )] string parameters,
			[MarshalAs( UnmanagedType.BStr )] string commonRoot,
			[MarshalAs( UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR )] string[] pathList,
			[MarshalAs( UnmanagedType.BStr )] string originalMessage );

		[return: MarshalAs( UnmanagedType.BStr )]
		string GetCommitMessage2( IntPtr hParentWnd,
			[MarshalAs( UnmanagedType.BStr )] string parameters,
			[MarshalAs( UnmanagedType.BStr )] string commonURL,
			[MarshalAs( UnmanagedType.BStr )] string commonRoot,
			[MarshalAs( UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR )] string[] pathList,
			[MarshalAs( UnmanagedType.BStr )] string originalMessage );

		[return: MarshalAs( UnmanagedType.BStr )]
		string OnCommitFinished(
			IntPtr hParentWnd,
			[MarshalAs( UnmanagedType.BStr )] string commonRoot,
			[MarshalAs( UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR )] string[] pathList,
			[MarshalAs( UnmanagedType.BStr )] string logMessage,
			[MarshalAs( UnmanagedType.U4 )] int revision );

		[return: MarshalAs( UnmanagedType.VariantBool )]
		bool HasOptions( );

		[return: MarshalAs( UnmanagedType.BStr )]
		string ShowOptionsDialog(
			IntPtr hParentWnd,
			[MarshalAs( UnmanagedType.BStr )] string parameters );
	} 
}
