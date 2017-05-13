namespace Interop.BugTraqProvider
{
    using System;
    using System.Runtime.InteropServices;

    // Original IDL definition can be found at:
    // https://sourceforge.net/p/tortoisesvn/code/HEAD/tree/trunk/contrib/issue-tracker-plugins/inc/IBugTraqProvider.idl?format=raw
    // https://sourceforge.net/p/tortoisesvn/code/HEAD/tree/trunk/contrib/issue-tracker-plugins/inc/IBugTraqProvider.idl
    //
    // See also:
    // https://tortoisegit.org/docs/tortoisegit/tgit-ibugtraqprovider-1.html
    // https://tortoisesvn.net/docs/release/TortoiseSVN_en/tsvn-ibugtraqprovider.html#tsvn-ibugtraqprovider-1
    //

    /// <remarks>
    /// While the rest of TortoiseSVN and TortoiseGit are licensed under the GPL,
    /// this portion is public domain.
    /// </remarks>

    [ ComImport ]
    [ InterfaceType(ComInterfaceType.InterfaceIsIUnknown) ]
    [ Guid("298B927C-7220-423C-B7B4-6E241F00CD93") ]
    public interface IBugTraqProvider
    {
        /// <summary>
        /// The ValidateParameters method is called from the settings
        /// dialog. This allows the provider to check that the parameters
        /// are OK. The provider can do basic syntax checking, it can check
        /// that the server is reachable, or it can do nothing.
        /// </summary>
        /// <param name="hParentWnd">
        /// Parent window for any UI that needs to be displayed during validation.</param>
        /// <param name="parameters">
        /// The parameter string that needs to be validated.</param>
        /// <returns>
        /// Is the string valid?</returns>
        /// <remarks>
        /// <para>
        /// A provider might need some parameters (e.g. a web service URL
        /// or a database connection string). This information is passed as
        /// a simple string. It's up to the individual provider to parse and
        /// validate it.</para>
        /// <para>
        /// If the provider needs to report a validation error, it should do this itself, using hParentWnd as
        /// the parent of any displayed UI.
        /// </para>
        /// <code>
        /// HRESULT ValidateParameters (
        ///     [in] HWND hParentWnd,               // Parent window for any UI that needs to be displayed during validation.
        ///     [in] BSTR parameters,               // The parameter string that needs to be validated.
        ///     [out, retval] VARIANT_BOOL *valid   // Is the string valid?
        /// );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.VariantBool)]
        bool ValidateParameters(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);

        /// <summary>
        /// In the commit dialog, the provider is accessed from a button. It
        /// needs to know what text to display (e.g. "Choose Bug" or
        /// "Select Ticket").
        /// </summary>
        /// <param name="hParentWnd">
        /// Parent window for any (error) UI that needs to be displayed.</param>
        /// <param name="parameters">
        /// The parameter string, just in case you need to talk to your web
        /// service (e.g.) to find out what the correct text is.</param>
        /// <returns>
        /// Text to display using the current thread locale.</returns>
        /// <remarks>
        /// <code>
        /// HRESULT GetLinkText (
        ///         [in] HWND hParentWnd,           // Parent window for any (error) UI that needs to be displayed.
        ///         [in] BSTR parameters,           // The parameter string, just in case you need to talk to your web
        ///                                         // service (e.g.) to find out what the correct text is.
        ///         [out, retval] BSTR *linkText    // What text do you want to display? Use the current thread locale.
        ///     );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetLinkText(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);

        /// <summary>
        /// Get the commit message. This would normally involve displaying a
        /// dialog with a list of the issues assigned to the current user.
        /// </summary>
        /// <param name="hParentWnd">
        /// Parent window for your provider's UI.</param>
        /// <param name="parameters">
        /// Parameters for your provider.</param>
        /// <param name="commonRoot"></param>
        /// <param name="pathList"></param>
        /// <param name="originalMessage">
        /// The text already present in the commit message.
        /// Your provider should include this text in the new message, where
        /// appropriate.</param>
        /// <returns>
        /// The new text for the commit message. This replaces the original
        /// message.</returns>
        /// <remarks>
        /// <code>
        /// HRESULT GetCommitMessage (
        ///     [in] HWND hParentWnd,           // Parent window for your provider's UI.
        ///     [in] BSTR parameters,           // Parameters for your provider.
        ///     [in] BSTR commonRoot,
        ///     [in] SAFEARRAY(BSTR) pathList,
        ///     [in] BSTR originalMessage,      // The text already present in the commit message.
        ///                                     // Your provider should include this text in the new message, where appropriate.
        ///     [out, retval] BSTR *newMessage  // The new text for the commit message. This replaces the original message.
        /// );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetCommitMessage(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters,
            [MarshalAs(UnmanagedType.BStr)] string commonRoot,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] string[] pathList,
            [MarshalAs(UnmanagedType.BStr)] string originalMessage);
    }

    // Original IDL definition can be found at:
    // http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/contrib/issue-tracker-plugins/inc/IBugTraqProvider.idl
    //
    // See also:
    // https://tortoisegit.org/docs/tortoisegit/tgit-ibugtraqprovider-2.html
    // https://tortoisesvn.net/docs/release/TortoiseSVN_en/tsvn-ibugtraqprovider-2.html
    //

    /// <remarks>
    /// While the rest of TortoiseSVN and TortoiseGit are licensed under the GPL,
    /// this portion is public domain.
    /// </remarks>

    [ ComImport ]
    [ InterfaceType(ComInterfaceType.InterfaceIsIUnknown) ]
    [ Guid("C5C85E31-2F9B-4916-A7BA-8E27D481EE83") ]
    public interface IBugTraqProvider2 : IBugTraqProvider
    {
        /// <summary>
        /// The ValidateParameters method is called from the settings
        /// dialog. This allows the provider to check that the parameters
        /// are OK. The provider can do basic syntax checking, it can check
        /// that the server is reachable, or it can do nothing.
        /// </summary>
        /// <param name="hParentWnd">
        /// Parent window for any UI that needs to be displayed during validation.</param>
        /// <param name="parameters">
        /// The parameter string that needs to be validated.</param>
        /// <returns>
        /// Is the string valid?</returns>
        /// <remarks>
        /// <para>
        /// A provider might need some parameters (e.g. a web service URL
        /// or a database connection string). This information is passed as
        /// a simple string. It's up to the individual provider to parse and
        /// validate it.</para>
        /// <para>
        /// If the provider needs to report a validation error, it should do this itself, using hParentWnd as
        /// the parent of any displayed UI.
        /// </para>
        /// <code>
        /// HRESULT ValidateParameters (
        ///     [in] HWND hParentWnd,               // Parent window for any UI that needs to be displayed during validation.
        ///     [in] BSTR parameters,               // The parameter string that needs to be validated.
        ///     [out, retval] VARIANT_BOOL *valid   // Is the string valid?
        /// );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.VariantBool)]
        new bool ValidateParameters(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);

        /// <summary>
        /// In the commit dialog, the provider is accessed from a button. It
        /// needs to know what text to display (e.g. "Choose Bug" or
        /// "Select Ticket").
        /// </summary>
        /// <param name="hParentWnd">
        /// Parent window for any (error) UI that needs to be displayed.</param>
        /// <param name="parameters">
        /// The parameter string, just in case you need to talk to your web
        /// service (e.g.) to find out what the correct text is.</param>
        /// <returns>
        /// Text to display using the current thread locale.</returns>
        /// <remarks>
        /// <code>
        /// HRESULT GetLinkText (
        ///         [in] HWND hParentWnd,           // Parent window for any (error) UI that needs to be displayed.
        ///         [in] BSTR parameters,           // The parameter string, just in case you need to talk to your web
        ///                                         // service (e.g.) to find out what the correct text is.
        ///         [out, retval] BSTR *linkText    // What text do you want to display? Use the current thread locale.
        ///     );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        new string GetLinkText(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);

        /// <summary>
        /// Get the commit message. This would normally involve displaying a
        /// dialog with a list of the issues assigned to the current user.
        /// </summary>
        /// <param name="hParentWnd">
        /// Parent window for your provider's UI.</param>
        /// <param name="parameters">
        /// Parameters for your provider.</param>
        /// <param name="commonRoot"></param>
        /// <param name="pathList"></param>
        /// <param name="originalMessage">
        /// The text already present in the commit message.
        /// Your provider should include this text in the new message, where
        /// appropriate.</param>
        /// <returns>
        /// The new text for the commit message. This replaces the original
        /// message.</returns>
        /// <remarks>
        /// <code>
        /// HRESULT GetCommitMessage (
        ///     [in] HWND hParentWnd,           // Parent window for your provider's UI.
        ///     [in] BSTR parameters,           // Parameters for your provider.
        ///     [in] BSTR commonRoot,
        ///     [in] SAFEARRAY(BSTR) pathList,
        ///     [in] BSTR originalMessage,      // The text already present in the commit message.
        ///                                     // Your provider should include this text in the new message, where appropriate.
        ///     [out, retval] BSTR *newMessage  // The new text for the commit message. This replaces the original message.
        /// );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        new string GetCommitMessage(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters,
            [MarshalAs(UnmanagedType.BStr)] string commonRoot,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] string[] pathList,
            [MarshalAs(UnmanagedType.BStr)] string originalMessage);

        /// <remarks>
        /// <code>
        /// HRESULT GetCommitMessage2 (
        ///         [in] HWND hParentWnd,                   // Parent window for your provider's UI.
        ///         [in] BSTR parameters,                   // Parameters for your provider.
        ///         [in] BSTR commonURL,                    // the common URL of the commit
        ///         [in] BSTR commonRoot,
        ///         [in] SAFEARRAY(BSTR) pathList,
        ///         [in] BSTR originalMessage,              // The text already present in the commit message.
        ///                                                 // Your provider should include this text in the new message, where appropriate.
        ///         // you can assign custom revision properties to a commit by setting the next two params.
        ///         // note: both safearrays must be of the same length. For every property name there must be a property value!
        ///         [in] BSTR bugID,                        // the content of the bugID field (if shown)
        ///         [out] BSTR * bugIDOut,                  // modified content of the bugID field
        ///         [out] SAFEARRAY(BSTR) * revPropNames,   // a list of revision property names which are applied to the commit
        ///         [out] SAFEARRAY(BSTR) * revPropValues,  // a list of revision property values which are applied to the commit
        ///         [out, retval] BSTR * newMessage         // The new text for the commit message. This replaces the original message.
        ///     );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        string GetCommitMessage2(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters,
            [MarshalAs(UnmanagedType.BStr)] string commonURL,
            [MarshalAs(UnmanagedType.BStr)] string commonRoot,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] string[] pathList,
            [MarshalAs(UnmanagedType.BStr)] string originalMessage,
            [MarshalAs(UnmanagedType.BStr)] string bugID,
            [MarshalAs(UnmanagedType.BStr)] out string bugIDOut,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] out string[] revPropNames,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] out string[] revPropValues);

        /// <remarks>
        /// <code>
        /// HRESULT CheckCommit (
        ///         [in] HWND hParentWnd,
        ///         [in] BSTR parameters,
        ///         [in] BSTR commonURL,
        ///         [in] BSTR commonRoot,
        ///         [in] SAFEARRAY(BSTR) pathList,
        ///         [in] BSTR commitMessage,
        ///         [out, retval] BSTR * errorMessage
        ///         );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        string CheckCommit(IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters,
            [MarshalAs(UnmanagedType.BStr)] string commonURL,
            [MarshalAs(UnmanagedType.BStr)] string commonRoot,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] string[] pathList,
            [MarshalAs(UnmanagedType.BStr)] string commitMessage);

        /// <remarks>
        /// <code>
        /// HRESULT OnCommitFinished (
        ///         [in] HWND hParentWnd,                   // Parent window for any (error) UI that needs to be displayed.
        ///         [in] BSTR commonRoot,                   // The common root of all paths that got committed.
        ///         [in] SAFEARRAY(BSTR) pathList,          // All the paths that got committed.
        ///         [in] BSTR logMessage,                   // The text already present in the commit message.
        ///         [in] ULONG revision,                    // The revision of the commit.
        ///         [out, retval] BSTR * error              // An error to show to the user if this function returns something else than S_OK
        ///         );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        string OnCommitFinished(
            IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string commonRoot,
            [MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)] string[] pathList,
            [MarshalAs(UnmanagedType.BStr)] string logMessage,
            [MarshalAs(UnmanagedType.U4)] int revision);

        /// <remarks>
        /// <code>
        /// HRESULT HasOptions(
        ///         [out, retval] VARIANT_BOOL *ret         // Whether the provider provides options
        ///         );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.VariantBool)]
        bool HasOptions();

        /// <remarks>
        /// <code>
        /// HRESULT ShowOptionsDialog(
        ///         [in] HWND hParentWnd,                   // Parent window for the options dialog
        ///         [in] BSTR parameters,                   // Parameters for your provider.
        ///         [out, retval] BSTR * newparameters      // the parameters string
        ///         );
        /// </code>
        /// </remarks>

        [return: MarshalAs(UnmanagedType.BStr)]
        string ShowOptionsDialog(
            IntPtr hParentWnd,
            [MarshalAs(UnmanagedType.BStr)] string parameters);
    }

}
