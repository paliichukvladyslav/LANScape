#ifndef SHELLEXT_HPP
#define SHELLEXT_HPP

#include <windows.h>
#include <shlobj.h>
#include <propkey.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <shellapi.h>
#include <nlohmann/json.hpp>
#include <new>

/* 099e168d-2180-4430-9a4c-34eb716023a0 */
const IID CLSID_LANScapeFolder = {0x099E168D, 0x2180, 0x4430, {0x9A, 0x4C, 0x34, 0xEB, 0x71, 0x60, 0x23, 0xA0}};

#define LANSCAPE_MAGIC_ID 0x4C53
const int g_nMaxLevel = 1;

#pragma pack(1)
typedef struct tagLANScapeObjectID {
    USHORT  cb;
    WORD    wMagic;
    BYTE    nLevel;
    BYTE    bIsOnline;
    WCHAR   szIpAddress[16];
    WCHAR   szMotd[128];
    WCHAR   szName[1];
} LANSCAPEITEMID;
#pragma pack()

typedef UNALIGNED LANSCAPEITEMID *PLANSCAPEITEMID;
typedef const UNALIGNED LANSCAPEITEMID *PCLANSCAPEITEMID;

class CLANScapeFolder;

class CLANScapeEnumIDList : public IEnumIDList {
    public:
        CLANScapeEnumIDList(CLANScapeFolder *pFolder, DWORD grfFlags, int nLevel);
        ~CLANScapeEnumIDList();

        IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();

        IFACEMETHODIMP Next(ULONG celt, PITEMID_CHILD *rgelt, ULONG *pceltFetched);
        IFACEMETHODIMP Skip(ULONG celt);
        IFACEMETHODIMP Reset();
        IFACEMETHODIMP Clone(IEnumIDList **ppenum);

    private:
        long m_cRef;
        CLANScapeFolder *m_pFolder;
        DWORD m_grfFlags;
        int m_nLevel;
        ULONG m_nItem;

        bool m_bFetched = false;
        std::vector<nlohmann::json> m_cachedNodes;
};

class CLANScapeFolder : public IShellFolder2, public IPersistFolder2 {
    public:
        CLANScapeFolder(int nLevel);
        ~CLANScapeFolder();

        IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();

        IFACEMETHODIMP GetClassID(CLSID *pClassID);

        IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl);

        IFACEMETHODIMP GetCurFolder(PIDLIST_ABSOLUTE *ppidl);

        IFACEMETHODIMP ParseDisplayName(HWND hwnd, IBindCtx *pbc, PWSTR pszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes);
        IFACEMETHODIMP EnumObjects(HWND hwnd, SHCONTF grfFlags, IEnumIDList **ppenumIDList);
        IFACEMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, IBindCtx *pbc, REFIID riid, void **ppv);
        IFACEMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, IBindCtx *pbc, REFIID riid, void **ppv);
        IFACEMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);
        IFACEMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv);
        IFACEMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut);
        IFACEMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *rgfReserved, void **ppv);
        IFACEMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pName);
        IFACEMETHODIMP SetNameOf(HWND hwnd, PCUITEMID_CHILD pidl, LPCWSTR pszName, SHGDNF uFlags, PITEMID_CHILD *ppidlOut);

        IFACEMETHODIMP GetDefaultSearchGUID(GUID *pguid);
        IFACEMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);
        IFACEMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
        IFACEMETHODIMP GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags);
        IFACEMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const PROPERTYKEY *pscid, VARIANT *pv);
        IFACEMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);
        IFACEMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

        HRESULT CreateChildID(PCWSTR pszName, PCWSTR pszIp, PCWSTR pszMotd, bool bIsOnline, int nLevel, PITEMID_CHILD *ppidl);

    private:
        long m_cRef;
        int m_nLevel;
        PIDLIST_ABSOLUTE m_pidl;
};

class CLANScapeIcon : public IExtractIconW {
    public:
        CLANScapeIcon(bool bIsOnline);

        IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
        IFACEMETHODIMP_(ULONG) AddRef();
        IFACEMETHODIMP_(ULONG) Release();

        IFACEMETHODIMP GetIconLocation(UINT uFlags, PWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
        IFACEMETHODIMP Extract(PCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    private:
        long m_cRef;
        bool m_bIsOnline;
};

#endif
