#include "shellext.hpp"
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <string>

long g_cRefModule = 0;

void DllAddRef() {
    InterlockedIncrement(&g_cRefModule);
}

void DllRelease() {
    InterlockedDecrement(&g_cRefModule);
}

std::wstring Utf8ToWString(const std::string &str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

CLANScapeIcon::CLANScapeIcon(bool bIsOnline) : m_cRef(1), m_bIsOnline(bIsOnline) {
    DllAddRef();
}

IFACEMETHODIMP CLANScapeIcon::QueryInterface(REFIID riid, void **ppv) {
    static const QITAB qit[] = {
        QITABENT(CLANScapeIcon, IExtractIconW),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CLANScapeIcon::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) CLANScapeIcon::Release() {
    long cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        DllRelease();
        delete this;
    }
    return cRef;
}

IFACEMETHODIMP CLANScapeIcon::GetIconLocation(UINT uFlags, PWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags) {
    StringCchCopyW(pszIconFile, cchMax, L"C:\\Windows\\System32\\shell32.dll");
    *piIndex = 15;
    *pwFlags = GIL_PERCLASS;
    return S_OK;
}

IFACEMETHODIMP CLANScapeIcon::Extract(PCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) {
    return S_FALSE;
}

CLANScapeEnumIDList::CLANScapeEnumIDList(CLANScapeFolder *pFolder, DWORD grfFlags, int nLevel) : m_cRef(1), m_pFolder(pFolder), m_grfFlags(grfFlags), m_nLevel(nLevel), m_nItem(0) {
    if (m_pFolder) m_pFolder->AddRef();
}

CLANScapeEnumIDList::~CLANScapeEnumIDList() {
    if (m_pFolder) m_pFolder->Release();
}

IFACEMETHODIMP CLANScapeEnumIDList::QueryInterface(REFIID riid, void **ppv) {
    static const QITAB qit[] = {
        QITABENT(CLANScapeEnumIDList, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CLANScapeEnumIDList::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) CLANScapeEnumIDList::Release() {
    long cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

IFACEMETHODIMP CLANScapeEnumIDList::Next(ULONG celt, PITEMID_CHILD *rgelt, ULONG *pceltFetched) {
    ULONG celtFetched = 0;

    if (!m_bFetched) {
        m_bFetched = true;
        try {
            zmq::context_t ctx(1);
            zmq::socket_t req(ctx, zmq::socket_type::req);
            req.connect("tcp://127.0.0.1:62627");

            zmq::message_t request("GET", 3);
            req.send(request, zmq::send_flags::none);

            zmq::message_t reply;
            auto res = req.recv(reply, zmq::recv_flags::none);
            if (res) {
                std::string json_str(static_cast<char *>(reply.data()), reply.size());
                auto nodes = nlohmann::json::parse(json_str);

                for (const auto &node : nodes) {
                    m_cachedNodes.push_back(node);
                }
            }
        }
        catch (...) {
            /* do nothing */
        }
    }

    while (celtFetched < celt && m_nItem < m_cachedNodes.size()) {
        const auto &node = m_cachedNodes[m_nItem];

        std::wstring name = Utf8ToWString(node.value("display_name", "Unknown"));
        std::wstring ip = Utf8ToWString(node.value("device_ip", "0.0.0.0"));
        bool isOnline = (node.value("status", "") == "online");

        std::wstring motd = Utf8ToWString(node.value("motd", ""));

        if (SUCCEEDED(m_pFolder->CreateChildID(name.c_str(), ip.c_str(), motd.c_str(), isOnline, m_nLevel, &rgelt[celtFetched]))) {
            celtFetched++;
            m_nItem++;
        } else {
            break;
        }
    }

    if (pceltFetched) {
        *pceltFetched = celtFetched;
    }

    return (celtFetched == celt) ? S_OK : S_FALSE;
}

IFACEMETHODIMP CLANScapeEnumIDList::Skip(ULONG celt) {
    m_nItem += celt;
    return S_OK;
}

IFACEMETHODIMP CLANScapeEnumIDList::Reset() {
    m_nItem = 0;
    m_bFetched = false;
    m_cachedNodes.clear();
    return S_OK;
}

IFACEMETHODIMP CLANScapeEnumIDList::Clone(IEnumIDList **ppenum) {
    *ppenum = new (std::nothrow) CLANScapeEnumIDList(m_pFolder, m_grfFlags, m_nLevel);
    return *ppenum ? S_OK : E_OUTOFMEMORY;
}

CLANScapeFolder::CLANScapeFolder(int nLevel) : m_cRef(1), m_nLevel(nLevel), m_pidl(NULL) {
    DllAddRef();
}

CLANScapeFolder::~CLANScapeFolder() {
    ILFree(m_pidl);
    DllRelease();
}

IFACEMETHODIMP CLANScapeFolder::QueryInterface(REFIID riid, void **ppv) {
    static const QITAB qit[] = {
        QITABENT(CLANScapeFolder, IShellFolder),
        QITABENT(CLANScapeFolder, IShellFolder2),
        QITABENT(CLANScapeFolder, IPersist),
        QITABENT(CLANScapeFolder, IPersistFolder),
        QITABENT(CLANScapeFolder, IPersistFolder2),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CLANScapeFolder::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) CLANScapeFolder::Release() {
    long cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

IFACEMETHODIMP CLANScapeFolder::GetClassID(CLSID *pClassID) {
    *pClassID = CLSID_LANScapeFolder;
    return S_OK;
}

IFACEMETHODIMP CLANScapeFolder::Initialize(PCIDLIST_ABSOLUTE pidl) {
    ILFree(m_pidl);
    m_pidl = ILClone(pidl);
    return m_pidl ? S_OK : E_OUTOFMEMORY;
}

IFACEMETHODIMP CLANScapeFolder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl) {
    if (!m_pidl) return S_FALSE;
    *ppidl = ILClone(m_pidl);
    return *ppidl ? S_OK : E_OUTOFMEMORY;
}

IFACEMETHODIMP CLANScapeFolder::ParseDisplayName(HWND, IBindCtx *, PWSTR, ULONG *, PIDLIST_RELATIVE *, ULONG *) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::EnumObjects(HWND, SHCONTF grfFlags, IEnumIDList **ppenumIDList) {
    *ppenumIDList = new (std::nothrow) CLANScapeEnumIDList(this, grfFlags, m_nLevel);
    return *ppenumIDList ? S_OK : E_OUTOFMEMORY;
}

IFACEMETHODIMP CLANScapeFolder::BindToObject(PCUIDLIST_RELATIVE, IBindCtx *, REFIID, void **) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::BindToStorage(PCUIDLIST_RELATIVE, IBindCtx *, REFIID, void **) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::CompareIDs(LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::CreateViewObject(HWND, REFIID riid, void **ppv) {
    if (riid == IID_IShellView) {
        SFV_CREATE csfv = { sizeof(csfv), 0 };
        csfv.pshf = this;
        return SHCreateShellFolderView(&csfv, (IShellView **)ppv);
    }
    return E_NOINTERFACE;
}

IFACEMETHODIMP CLANScapeFolder::GetAttributesOf(UINT, PCUITEMID_CHILD_ARRAY, SFGAOF *rgfInOut) {
    if (rgfInOut) *rgfInOut &= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;
    return S_OK;
}

IFACEMETHODIMP CLANScapeFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *rgfReserved, void **ppv) {
    if (!ppv) return E_POINTER;
    *ppv = NULL;

    if (cidl < 1 || !apidl) return E_INVALIDARG;

    PCLANSCAPEITEMID pItem = (PCLANSCAPEITEMID)apidl[0];
    if (pItem->wMagic != LANSCAPE_MAGIC_ID) return E_INVALIDARG;

    if (riid == IID_IExtractIconW) {
        *ppv = new (std::nothrow) CLANScapeIcon(pItem->bIsOnline != 0);
        return *ppv ? S_OK : E_OUTOFMEMORY;
    }

    return E_NOINTERFACE;
}

IFACEMETHODIMP CLANScapeFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, SHGDNF, STRRET *pName) {
    if (!pidl || !pName) return E_INVALIDARG;
    PCLANSCAPEITEMID pItem = (PCLANSCAPEITEMID)pidl;
    if (pItem->wMagic != LANSCAPE_MAGIC_ID) return E_INVALIDARG;
    pName->uType = STRRET_WSTR;
    return SHStrDupW((LPCWSTR)(void *)pItem->szName, &pName->pOleStr);
}

IFACEMETHODIMP CLANScapeFolder::SetNameOf(HWND, PCUITEMID_CHILD, LPCWSTR, SHGDNF, PITEMID_CHILD *) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::GetDefaultSearchGUID(GUID *) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::EnumSearches(IEnumExtraSearch **) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::GetDefaultColumn(DWORD, ULONG *pSort, ULONG *pDisplay) {
    if (pSort) *pSort = 0;
    if (pDisplay) *pDisplay = 0;
    return S_OK;
}

IFACEMETHODIMP CLANScapeFolder::GetDefaultColumnState(UINT, SHCOLSTATEF *pcsFlags) {
    if (!pcsFlags) return E_INVALIDARG;
    *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
    return S_OK;
}

IFACEMETHODIMP CLANScapeFolder::GetDetailsEx(PCUITEMID_CHILD, const PROPERTYKEY *, VARIANT *) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CLANScapeFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) {
    if (!psd) return E_INVALIDARG;

    if (!pidl) {
        psd->fmt = LVCFMT_LEFT;
        psd->str.uType = STRRET_WSTR;
        if (iColumn == 0) SHStrDupW(L"Name", &psd->str.pOleStr);
        else if (iColumn == 1) SHStrDupW(L"IP Address", &psd->str.pOleStr);
        else if (iColumn == 2) SHStrDupW(L"Status", &psd->str.pOleStr);
        else if (iColumn == 3) SHStrDupW(L"MOTD", &psd->str.pOleStr);
        else return E_FAIL;
        return S_OK;
    }

    PCLANSCAPEITEMID pItem = (PCLANSCAPEITEMID)pidl;
    if (pItem->wMagic != LANSCAPE_MAGIC_ID) return E_INVALIDARG;

    psd->str.uType = STRRET_WSTR;

    if (iColumn == 0) SHStrDupW((LPCWSTR)(void *)pItem->szName, &psd->str.pOleStr);
    else if (iColumn == 1) SHStrDupW((LPCWSTR)(void *)pItem->szIpAddress, &psd->str.pOleStr);
    else if (iColumn == 2) SHStrDupW(pItem->bIsOnline ? L"Online" : L"Offline", &psd->str.pOleStr);
    else if (iColumn == 3) SHStrDupW((LPCWSTR)(void *)pItem->szMotd, &psd->str.pOleStr);
    else return E_FAIL;

    return S_OK;
}

IFACEMETHODIMP CLANScapeFolder::MapColumnToSCID(UINT, SHCOLUMNID *) {
    return E_NOTIMPL;
}

HRESULT CLANScapeFolder::CreateChildID(PCWSTR pszName, PCWSTR pszIp, PCWSTR pszMotd, bool bIsOnline, int nLevel, PITEMID_CHILD *ppidl) {
    if (!ppidl) return E_INVALIDARG;
    size_t cchName = 0, cchIp = 0;
    StringCchLengthW(pszName, STRSAFE_MAX_CCH, &cchName);
    StringCchLengthW(pszIp, STRSAFE_MAX_CCH, &cchIp);
    USHORT cbBase = sizeof(LANSCAPEITEMID) - sizeof(WCHAR);
    USHORT cbName = (USHORT)((cchName + 1) * sizeof(WCHAR));
    USHORT cb = cbBase + cbName;
    PLANSCAPEITEMID pidl = (PLANSCAPEITEMID)CoTaskMemAlloc(cb + sizeof(USHORT));
    if (!pidl) return E_OUTOFMEMORY;
    ZeroMemory(pidl, cb + sizeof(USHORT));
    pidl->cb = cb;
    pidl->wMagic = LANSCAPE_MAGIC_ID;
    pidl->nLevel = (BYTE)nLevel;
    pidl->bIsOnline = bIsOnline ? 1 : 0;

    StringCchCopyW((LPWSTR)(void *)pidl->szIpAddress, ARRAYSIZE(pidl->szIpAddress), pszIp);

    if (pszMotd) {
        StringCchCopyW((LPWSTR)(void *)pidl->szMotd, ARRAYSIZE(pidl->szMotd), pszMotd);
    }

    StringCchCopyW((LPWSTR)(void *)pidl->szName, cchName + 1, pszName);

    *ppidl = (PITEMID_CHILD)pidl;
    return S_OK;
}

class CLANScapeClassFactory : public IClassFactory {
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory *>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IFACEMETHODIMP_(ULONG) AddRef() { return 2; }
    IFACEMETHODIMP_(ULONG) Release() { return 1; }

    IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) {
        if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;
        CLANScapeFolder *pExt = new (std::nothrow) CLANScapeFolder(0);
        if (pExt == NULL) return E_OUTOFMEMORY;
        HRESULT hr = pExt->QueryInterface(riid, ppv);
        pExt->Release();
        return hr;
    }

    IFACEMETHODIMP LockServer(BOOL fLock) {
        if (fLock) DllAddRef();
        else DllRelease();
        return S_OK;
    }
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) {
    if (rclsid == CLSID_LANScapeFolder) {
        static CLANScapeClassFactory factory;
        return factory.QueryInterface(riid, ppv);
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void) {
    return g_cRefModule == 0 ? S_OK : S_FALSE;
}
