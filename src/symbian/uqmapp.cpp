#include <coecntrl.h>
#include <aknappui.h>
#include <aknapp.h>
#include <akndoc.h>
#include <sdlepocapi.h>
#include <aknnotewrappers.h>
#include <eikstart.h>
#include <badesca.h>
#include <bautils.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL_keysym.h>

const TUid KUidSdlApp={ 0xA000A0C3 };

class CSDL;

class CSDLObserver : public CBase, public MSDLObserver
{
public:
    CSDLObserver(CSDL* aSdl);

    TInt SdlEvent(TInt aEvent, TInt aParam);
    TInt SdlThreadEvent(TInt aEvent, TInt aParam);

private:
    CSDL* iSdl;    
};

class MExitWait
    {
    public:
        virtual void DoExit(TInt aErr) = 0;
    };   

class CExitWait : public CActive
    {
    public:
        CExitWait(MExitWait& aWait);
        void Start();
        ~CExitWait();
    private:
        void RunL();
        void DoCancel();
    private:
        MExitWait& iWait;
        TRequestStatus* iStatusPtr;
    };

class CSDLWin : public CCoeControl
    {
    public:
        void ConstructL(const TRect& aRect);
        RWindow& GetWindow() const;
        void SetNoDraw();
    private:
        void Draw(const TRect& aRect) const;
    };  
    
class CSdlApplication : public CAknApplication
    {
private:
    // from CApaApplication
    CApaDocument* CreateDocumentL();
    TUid AppDllUid() const;
    };
    
    
class CSdlAppDocument : public CAknDocument
    {
public:
    CSdlAppDocument(CEikApplication& aApp): CAknDocument(aApp) { }
private:
    CEikAppUi* CreateAppUiL();
    };
    
            
class CSdlAppUi : public CAknAppUi, public MExitWait
    {
public:
    void ConstructL();
    ~CSdlAppUi();
private:
    void HandleCommandL(TInt aCommand);
    void DoExit(TInt aErr);
    void HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination);
    void HandleResourceChangeL(TInt aType);
    void AddCmdLineParamsL(CDesC8Array& aArgs);
private:
    CExitWait* iWait;
    CSDLWin* iSDLWin;
    CSDL* iSdl;
    CSDLObserver* iSdlObserver;
    TBool iExit;
    };  


CExitWait::CExitWait(MExitWait& aWait) : CActive(CActive::EPriorityStandard), iWait(aWait)
    {
    CActiveScheduler::Add(this);
    }
    
CExitWait::~CExitWait()
    {
    Cancel();
    }
 
void CExitWait::RunL()
    {
    if(iStatusPtr != NULL )
        iWait.DoExit(iStatus.Int());
    }
    
void CExitWait::DoCancel()
    {
    if(iStatusPtr != NULL )
        User::RequestComplete(iStatusPtr , KErrCancel);
    }
    
void CExitWait::Start()
    {
    SetActive();
    iStatusPtr = &iStatus;
    }

void CSDLWin:: ConstructL(const TRect& aRect)   
    {
    CreateWindowL();
    SetRect(aRect);
    ActivateL();
    }

    
RWindow& CSDLWin::GetWindow() const
    {
    return Window();
    }
    

void CSDLWin::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ESolidPen);
    gc.SetPenColor(KRgbBlack);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.SetBrushColor(0x000000);
    gc.DrawRect(Rect());
    }   
    
void CSdlAppUi::ConstructL()    
    {
    BaseConstructL(ENoScreenFurniture | EAppOrientationLandscape);
        
    iSDLWin = new (ELeave) CSDLWin;
    iSDLWin->ConstructL(ApplicationRect());
                
    iSdl = CSDL::NewL(CSDL::EEnableFocusStop);
    iSdlObserver = new (ELeave) CSDLObserver(iSdl);
    
    iSdl->SetContainerWindowL(
                    iSDLWin->GetWindow(), 
                    iEikonEnv->WsSession(),
                    *iEikonEnv->ScreenDevice());    
    iSdl->SetObserver(iSdlObserver);
    iSdl->DisableKeyBlocking(*this);    
    
    iWait = new (ELeave) CExitWait(*this);    
    CDesC8ArrayFlat* args = new (ELeave)CDesC8ArrayFlat(10);
    AddCmdLineParamsL(*args);
    
    iSdl->CallMainL(iWait->iStatus, *args, CSDL::ENoFlags, 81920);
    delete args;
    
    iWait->Start();     
    }

void CSdlAppUi::HandleCommandL(TInt aCommand)
    {
    switch(aCommand)
        {
        case EAknCmdExit:
        case EAknSoftkeyExit:
        case EEikCmdExit:
            exit(0);
            break;
            
        default:
            break;
        }
    }
    
void CSdlAppUi::DoExit(TInt aErr)
    {       
    delete iSdl;
    iSdl = NULL;
    
    if(iExit)
        Exit();
    }
    
void CSdlAppUi::HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination)
    {
    if(iSdl != NULL)
        iSdl->AppendWsEvent(aEvent);
    CAknAppUi::HandleWsEventL(aEvent, aDestination);
    }
    
void CSdlAppUi::HandleResourceChangeL(TInt aType)
    {
    CAknAppUi::HandleResourceChangeL(aType);
    
    if(aType == KEikDynamicLayoutVariantSwitch)
        {
        iSDLWin->SetRect(ApplicationRect());
        if (iSdl)
            {
            iSdl->SetContainerWindowL(
                        iSDLWin->GetWindow(),
                        iEikonEnv->WsSession(),
                        *iEikonEnv->ScreenDevice());
            }                       
        }
    }

void CSdlAppUi::AddCmdLineParamsL(CDesC8Array& aArgs)
    {       
    _LIT8(KAddonParam, "--addondir=?:\\uqm-addons");
    _LIT16(KTestFolder, "?:\\uqm-addons");
    RFs fs;
    
    fs.Connect();   
    for (TInt8 c = 'e'; c < 'z'; ++c)
        {
        TBuf16<32> buf(KTestFolder);
        buf[0] = c;
        if (BaflUtils::FolderExists(fs, buf))
            {
            TBuf8<32> arg(KAddonParam);
            arg[11] = c;
            aArgs.AppendL(arg);
            break;
            }
        }
    fs.Close();
    }

CSdlAppUi::~CSdlAppUi()
    {
    if(iWait != NULL)
        iWait->Cancel();
    delete iSdl;
    delete iWait;
    delete iSDLWin;
    delete iSdlObserver;
    }

CEikAppUi* CSdlAppDocument::CreateAppUiL()
    {
    return new(ELeave) CSdlAppUi();
    }   
    
TUid CSdlApplication::AppDllUid() const
    {
    return KUidSdlApp;
    }   
    

CApaDocument* CSdlApplication::CreateDocumentL()
    {
    CSdlAppDocument* document = new (ELeave) CSdlAppDocument(*this);
    return document;
    }
  
LOCAL_C CApaApplication* NewApplication()
    {
    return new CSdlApplication;
    }

GLDEF_C TInt E32Main()
    {
    return EikStart::RunApplication(NewApplication);
    }

CSDLObserver::CSDLObserver(CSDL* aSdl) : iSdl(aSdl)
{
}

TInt CSDLObserver::SdlEvent(TInt aEvent, TInt aParam)
{
    if (aEvent == EEventKeyMapInit)
    {
        // starmap zoom
        iSdl->SetSDLCode('3', SDLK_KP_PLUS);
        iSdl->SetSDLCode('2', SDLK_KP_MINUS);
        
        iSdl->SetSDLCode('A', SDLK_KP_PLUS);
        iSdl->SetSDLCode('Z', SDLK_KP_MINUS);
        
        iSdl->SetSDLCode('a', SDLK_KP_PLUS);
        iSdl->SetSDLCode('z', SDLK_KP_MINUS);        
    }
    return 0;
}

TInt CSDLObserver::SdlThreadEvent(TInt aEvent, TInt aParam)
{
    return 0;
}
