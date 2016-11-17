// dllmain.h : Declaration of module class.

class CExampleAtlPluginModule : public CAtlDllModuleT< CExampleAtlPluginModule >
{
public :
    DECLARE_LIBID(LIBID_ExampleAtlPluginLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_EXAMPLEATLPLUGIN, "{68BD4B94-C38D-4C03-8101-75176E711DC1}")
};

extern class CExampleAtlPluginModule _AtlModule;
