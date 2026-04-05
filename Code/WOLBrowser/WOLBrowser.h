/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Oct 22 14:51:58 2001
 */
/* Compiler settings for .\Code\WOLBrowser.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440

#include <cstdint>
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __WOLBrowser_h__
#define __WOLBrowser_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWOLBrowserEvent_FWD_DEFINED__
#define __IWOLBrowserEvent_FWD_DEFINED__
typedef interface IWOLBrowserEvent IWOLBrowserEvent;
#endif 	/* __IWOLBrowserEvent_FWD_DEFINED__ */


#ifndef __IWOLBrowser_FWD_DEFINED__
#define __IWOLBrowser_FWD_DEFINED__
typedef interface IWOLBrowser IWOLBrowser;
#endif 	/* __IWOLBrowser_FWD_DEFINED__ */


#ifndef __IWOLBrowserScript_FWD_DEFINED__
#define __IWOLBrowserScript_FWD_DEFINED__
typedef interface IWOLBrowserScript IWOLBrowserScript;
#endif 	/* __IWOLBrowserScript_FWD_DEFINED__ */


#ifndef __WOLBrowser_FWD_DEFINED__
#define __WOLBrowser_FWD_DEFINED__

#ifdef __cplusplus
typedef class WOLBrowser WOLBrowser;
#else
typedef struct WOLBrowser WOLBrowser;
#endif /* __cplusplus */

#endif 	/* __WOLBrowser_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IWOLBrowserEvent_INTERFACE_DEFINED__
#define __IWOLBrowserEvent_INTERFACE_DEFINED__

/* interface IWOLBrowserEvent */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IWOLBrowserEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28CC9F2D-1F5F-4d3f-A8C6-0C8C2172D68D")
    IWOLBrowserEvent : public IUnknown
    {
    public:
        virtual int32_t STDMETHODCALLTYPE OnScriptQuit( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnBeforeNavigate( 
            const wchar_t __RPC_FAR *url,
            const wchar_t __RPC_FAR *targetFrame) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnDocumentComplete( 
            const wchar_t __RPC_FAR *url,
            int32_t isTopFrame) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnDownloadBegin( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnProgressChange( 
            int32_t progress,
            int32_t progressMax) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnDownloadComplete( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnNavigateComplete( 
            const wchar_t __RPC_FAR *url) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnStatusTextChange( 
            const wchar_t __RPC_FAR *statusText) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnTitleChange( 
            const wchar_t __RPC_FAR *title) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnNewWindow( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnShowMessage( 
            const wchar_t __RPC_FAR *text,
            const wchar_t __RPC_FAR *caption,
            uint32_t type,
            int32_t __RPC_FAR *result) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnFailedPageCertification( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnErrorMsg( 
            const wchar_t __RPC_FAR *error) = 0;
        
        virtual int32_t STDMETHODCALLTYPE OnRegisterLogin( 
            const wchar_t __RPC_FAR *nick,
            const wchar_t __RPC_FAR *pass) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWOLBrowserEventVtbl
    {
        BEGIN_INTERFACE
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWOLBrowserEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        uint32_t ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        uint32_t ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnScriptQuit )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnBeforeNavigate )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *url,
            const wchar_t __RPC_FAR *targetFrame);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnDocumentComplete )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *url,
            int32_t isTopFrame);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnDownloadBegin )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnProgressChange )( 
            IWOLBrowserEvent __RPC_FAR * This,
            int32_t progress,
            int32_t progressMax);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnDownloadComplete )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnNavigateComplete )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *url);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnStatusTextChange )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *statusText);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnTitleChange )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *title);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnNewWindow )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnShowMessage )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *text,
            const wchar_t __RPC_FAR *caption,
            uint32_t type,
            int32_t __RPC_FAR *result);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnFailedPageCertification )( 
            IWOLBrowserEvent __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnErrorMsg )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *error);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *OnRegisterLogin )( 
            IWOLBrowserEvent __RPC_FAR * This,
            const wchar_t __RPC_FAR *nick,
            const wchar_t __RPC_FAR *pass);
        
        END_INTERFACE
    } IWOLBrowserEventVtbl;

    interface IWOLBrowserEvent
    {
        CONST_VTBL struct IWOLBrowserEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWOLBrowserEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWOLBrowserEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWOLBrowserEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWOLBrowserEvent_OnScriptQuit(This)	\
    (This)->lpVtbl -> OnScriptQuit(This)

#define IWOLBrowserEvent_OnBeforeNavigate(This,url,targetFrame)	\
    (This)->lpVtbl -> OnBeforeNavigate(This,url,targetFrame)

#define IWOLBrowserEvent_OnDocumentComplete(This,url,isTopFrame)	\
    (This)->lpVtbl -> OnDocumentComplete(This,url,isTopFrame)

#define IWOLBrowserEvent_OnDownloadBegin(This)	\
    (This)->lpVtbl -> OnDownloadBegin(This)

#define IWOLBrowserEvent_OnProgressChange(This,progress,progressMax)	\
    (This)->lpVtbl -> OnProgressChange(This,progress,progressMax)

#define IWOLBrowserEvent_OnDownloadComplete(This)	\
    (This)->lpVtbl -> OnDownloadComplete(This)

#define IWOLBrowserEvent_OnNavigateComplete(This,url)	\
    (This)->lpVtbl -> OnNavigateComplete(This,url)

#define IWOLBrowserEvent_OnStatusTextChange(This,statusText)	\
    (This)->lpVtbl -> OnStatusTextChange(This,statusText)

#define IWOLBrowserEvent_OnTitleChange(This,title)	\
    (This)->lpVtbl -> OnTitleChange(This,title)

#define IWOLBrowserEvent_OnNewWindow(This)	\
    (This)->lpVtbl -> OnNewWindow(This)

#define IWOLBrowserEvent_OnShowMessage(This,text,caption,type,result)	\
    (This)->lpVtbl -> OnShowMessage(This,text,caption,type,result)

#define IWOLBrowserEvent_OnFailedPageCertification(This)	\
    (This)->lpVtbl -> OnFailedPageCertification(This)

#define IWOLBrowserEvent_OnErrorMsg(This,error)	\
    (This)->lpVtbl -> OnErrorMsg(This,error)

#define IWOLBrowserEvent_OnRegisterLogin(This,nick,pass)	\
    (This)->lpVtbl -> OnRegisterLogin(This,nick,pass)

#endif /* COBJMACROS */


#endif 	/* C style interface */



int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnScriptQuit_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This);


void __RPC_STUB IWOLBrowserEvent_OnScriptQuit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnBeforeNavigate_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *url,
    const wchar_t __RPC_FAR *targetFrame);


void __RPC_STUB IWOLBrowserEvent_OnBeforeNavigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnDocumentComplete_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *url,
    int32_t isTopFrame);


void __RPC_STUB IWOLBrowserEvent_OnDocumentComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnDownloadBegin_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This);


void __RPC_STUB IWOLBrowserEvent_OnDownloadBegin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnProgressChange_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    int32_t progress,
    int32_t progressMax);


void __RPC_STUB IWOLBrowserEvent_OnProgressChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnDownloadComplete_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This);


void __RPC_STUB IWOLBrowserEvent_OnDownloadComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnNavigateComplete_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *url);


void __RPC_STUB IWOLBrowserEvent_OnNavigateComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnStatusTextChange_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *statusText);


void __RPC_STUB IWOLBrowserEvent_OnStatusTextChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnTitleChange_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *title);


void __RPC_STUB IWOLBrowserEvent_OnTitleChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnNewWindow_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This);


void __RPC_STUB IWOLBrowserEvent_OnNewWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnShowMessage_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *text,
    const wchar_t __RPC_FAR *caption,
    uint32_t type,
    int32_t __RPC_FAR *result);


void __RPC_STUB IWOLBrowserEvent_OnShowMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnFailedPageCertification_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This);


void __RPC_STUB IWOLBrowserEvent_OnFailedPageCertification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnErrorMsg_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *error);


void __RPC_STUB IWOLBrowserEvent_OnErrorMsg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserEvent_OnRegisterLogin_Proxy( 
    IWOLBrowserEvent __RPC_FAR * This,
    const wchar_t __RPC_FAR *nick,
    const wchar_t __RPC_FAR *pass);


void __RPC_STUB IWOLBrowserEvent_OnRegisterLogin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);



#endif 	/* __IWOLBrowserEvent_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_WOLBrowser_0209 */
/* [local] */ 


enum __MIDL___MIDL_itf_WOLBrowser_0209_0001
    {	UIFLAG_CONTEXTMENU	= 1,
	UIFLAG_POPUPWINDOWS	= 2,
	UIFLAG_TEXTSELECT	= 4,
	UIFLAG_3DBORDERS	= 8,
	UIFLAG_SCROLLBARS	= 16,
	UIFLAG_OPENNEWWINDOW	= 32
    };


extern RPC_IF_HANDLE __MIDL_itf_WOLBrowser_0209_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_WOLBrowser_0209_v0_0_s_ifspec;

#ifndef __IWOLBrowser_INTERFACE_DEFINED__
#define __IWOLBrowser_INTERFACE_DEFINED__

/* interface IWOLBrowser */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IWOLBrowser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0315484A-DC04-47a5-8F50-CDB7402425C5")
    IWOLBrowser : public IUnknown
    {
    public:
        virtual int32_t STDMETHODCALLTYPE Startup( 
            HWND parent,
            LPRECT rect) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE AddObserver( 
            IWOLBrowserEvent __RPC_FAR *observer) = 0;
        
        virtual int32_t STDMETHODCALLTYPE RemoveObserver( 
            IWOLBrowserEvent __RPC_FAR *observer) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Show( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Hide( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Reposition( 
            LPRECT rect) = 0;
        
        virtual int32_t STDMETHODCALLTYPE SetUIFlags( 
            uint32_t uiFlags) = 0;
        
        virtual int32_t STDMETHODCALLTYPE SetDownloadControl( 
            uint32_t dlFlags) = 0;
        
        virtual int32_t STDMETHODCALLTYPE ExecuteScript( 
            const wchar_t __RPC_FAR *script,
            DISPPARAMS __RPC_FAR *params,
            VARIANT __RPC_FAR *result) = 0;
        
        virtual int32_t STDMETHODCALLTYPE ExecuteScriptInFrame( 
            const wchar_t __RPC_FAR *frame,
            const wchar_t __RPC_FAR *script,
            DISPPARAMS __RPC_FAR *params,
            VARIANT __RPC_FAR *result) = 0;
        
        virtual int32_t STDMETHODCALLTYPE SetPageCertificate( 
            const wchar_t __RPC_FAR *certificate) = 0;
        
        virtual int32_t STDMETHODCALLTYPE AllowPageCertification( 
            int32_t allow) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Navigate( 
            const wchar_t __RPC_FAR *url,
            int32_t flags,
            const wchar_t __RPC_FAR *targetFrame) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE GoBack( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE GoForward( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE GoHome( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE GoSearch( void) = 0;
        
        virtual int32_t STDMETHODCALLTYPE GetLocationName( 
            wchar_t __RPC_FAR *locationName,
            int32_t size) = 0;
        
        virtual int32_t STDMETHODCALLTYPE GetLocationURL( 
            wchar_t __RPC_FAR *locationURL,
            int32_t size) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWOLBrowserVtbl
    {
        BEGIN_INTERFACE
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWOLBrowser __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        uint32_t ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWOLBrowser __RPC_FAR * This);
        
        uint32_t ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Startup )( 
            IWOLBrowser __RPC_FAR * This,
            HWND parent,
            LPRECT rect);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Shutdown )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *AddObserver )( 
            IWOLBrowser __RPC_FAR * This,
            IWOLBrowserEvent __RPC_FAR *observer);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *RemoveObserver )( 
            IWOLBrowser __RPC_FAR * This,
            IWOLBrowserEvent __RPC_FAR *observer);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Show )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Hide )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Reposition )( 
            IWOLBrowser __RPC_FAR * This,
            LPRECT rect);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *SetUIFlags )( 
            IWOLBrowser __RPC_FAR * This,
            uint32_t uiFlags);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *SetDownloadControl )( 
            IWOLBrowser __RPC_FAR * This,
            uint32_t dlFlags);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *ExecuteScript )( 
            IWOLBrowser __RPC_FAR * This,
            const wchar_t __RPC_FAR *script,
            DISPPARAMS __RPC_FAR *params,
            VARIANT __RPC_FAR *result);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *ExecuteScriptInFrame )( 
            IWOLBrowser __RPC_FAR * This,
            const wchar_t __RPC_FAR *frame,
            const wchar_t __RPC_FAR *script,
            DISPPARAMS __RPC_FAR *params,
            VARIANT __RPC_FAR *result);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *SetPageCertificate )( 
            IWOLBrowser __RPC_FAR * This,
            const wchar_t __RPC_FAR *certificate);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *AllowPageCertification )( 
            IWOLBrowser __RPC_FAR * This,
            int32_t allow);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Navigate )( 
            IWOLBrowser __RPC_FAR * This,
            const wchar_t __RPC_FAR *url,
            int32_t flags,
            const wchar_t __RPC_FAR *targetFrame);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GoBack )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GoForward )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GoHome )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GoSearch )( 
            IWOLBrowser __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GetLocationName )( 
            IWOLBrowser __RPC_FAR * This,
            wchar_t __RPC_FAR *locationName,
            int32_t size);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GetLocationURL )( 
            IWOLBrowser __RPC_FAR * This,
            wchar_t __RPC_FAR *locationURL,
            int32_t size);
        
        END_INTERFACE
    } IWOLBrowserVtbl;

    interface IWOLBrowser
    {
        CONST_VTBL struct IWOLBrowserVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWOLBrowser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWOLBrowser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWOLBrowser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWOLBrowser_Startup(This,parent,rect)	\
    (This)->lpVtbl -> Startup(This,parent,rect)

#define IWOLBrowser_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define IWOLBrowser_AddObserver(This,observer)	\
    (This)->lpVtbl -> AddObserver(This,observer)

#define IWOLBrowser_RemoveObserver(This,observer)	\
    (This)->lpVtbl -> RemoveObserver(This,observer)

#define IWOLBrowser_Show(This)	\
    (This)->lpVtbl -> Show(This)

#define IWOLBrowser_Hide(This)	\
    (This)->lpVtbl -> Hide(This)

#define IWOLBrowser_Reposition(This,rect)	\
    (This)->lpVtbl -> Reposition(This,rect)

#define IWOLBrowser_SetUIFlags(This,uiFlags)	\
    (This)->lpVtbl -> SetUIFlags(This,uiFlags)

#define IWOLBrowser_SetDownloadControl(This,dlFlags)	\
    (This)->lpVtbl -> SetDownloadControl(This,dlFlags)

#define IWOLBrowser_ExecuteScript(This,script,params,result)	\
    (This)->lpVtbl -> ExecuteScript(This,script,params,result)

#define IWOLBrowser_ExecuteScriptInFrame(This,frame,script,params,result)	\
    (This)->lpVtbl -> ExecuteScriptInFrame(This,frame,script,params,result)

#define IWOLBrowser_SetPageCertificate(This,certificate)	\
    (This)->lpVtbl -> SetPageCertificate(This,certificate)

#define IWOLBrowser_AllowPageCertification(This,allow)	\
    (This)->lpVtbl -> AllowPageCertification(This,allow)

#define IWOLBrowser_Navigate(This,url,flags,targetFrame)	\
    (This)->lpVtbl -> Navigate(This,url,flags,targetFrame)

#define IWOLBrowser_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IWOLBrowser_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IWOLBrowser_GoBack(This)	\
    (This)->lpVtbl -> GoBack(This)

#define IWOLBrowser_GoForward(This)	\
    (This)->lpVtbl -> GoForward(This)

#define IWOLBrowser_GoHome(This)	\
    (This)->lpVtbl -> GoHome(This)

#define IWOLBrowser_GoSearch(This)	\
    (This)->lpVtbl -> GoSearch(This)

#define IWOLBrowser_GetLocationName(This,locationName,size)	\
    (This)->lpVtbl -> GetLocationName(This,locationName,size)

#define IWOLBrowser_GetLocationURL(This,locationURL,size)	\
    (This)->lpVtbl -> GetLocationURL(This,locationURL,size)

#endif /* COBJMACROS */


#endif 	/* C style interface */



int32_t STDMETHODCALLTYPE IWOLBrowser_Startup_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    HWND parent,
    LPRECT rect);


void __RPC_STUB IWOLBrowser_Startup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Shutdown_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_AddObserver_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    IWOLBrowserEvent __RPC_FAR *observer);


void __RPC_STUB IWOLBrowser_AddObserver_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_RemoveObserver_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    IWOLBrowserEvent __RPC_FAR *observer);


void __RPC_STUB IWOLBrowser_RemoveObserver_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Show_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_Show_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Hide_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_Hide_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Reposition_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    LPRECT rect);


void __RPC_STUB IWOLBrowser_Reposition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_SetUIFlags_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    uint32_t uiFlags);


void __RPC_STUB IWOLBrowser_SetUIFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_SetDownloadControl_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    uint32_t dlFlags);


void __RPC_STUB IWOLBrowser_SetDownloadControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_ExecuteScript_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    const wchar_t __RPC_FAR *script,
    DISPPARAMS __RPC_FAR *params,
    VARIANT __RPC_FAR *result);


void __RPC_STUB IWOLBrowser_ExecuteScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_ExecuteScriptInFrame_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    const wchar_t __RPC_FAR *frame,
    const wchar_t __RPC_FAR *script,
    DISPPARAMS __RPC_FAR *params,
    VARIANT __RPC_FAR *result);


void __RPC_STUB IWOLBrowser_ExecuteScriptInFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_SetPageCertificate_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    const wchar_t __RPC_FAR *certificate);


void __RPC_STUB IWOLBrowser_SetPageCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_AllowPageCertification_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    int32_t allow);


void __RPC_STUB IWOLBrowser_AllowPageCertification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Navigate_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    const wchar_t __RPC_FAR *url,
    int32_t flags,
    const wchar_t __RPC_FAR *targetFrame);


void __RPC_STUB IWOLBrowser_Navigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Stop_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_Refresh_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_GoBack_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_GoBack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_GoForward_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_GoForward_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_GoHome_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_GoHome_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_GoSearch_Proxy( 
    IWOLBrowser __RPC_FAR * This);


void __RPC_STUB IWOLBrowser_GoSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_GetLocationName_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    wchar_t __RPC_FAR *locationName,
    int32_t size);


void __RPC_STUB IWOLBrowser_GetLocationName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowser_GetLocationURL_Proxy( 
    IWOLBrowser __RPC_FAR * This,
    wchar_t __RPC_FAR *locationURL,
    int32_t size);


void __RPC_STUB IWOLBrowser_GetLocationURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);



#endif 	/* __IWOLBrowser_INTERFACE_DEFINED__ */


#ifndef __IWOLBrowserScript_INTERFACE_DEFINED__
#define __IWOLBrowserScript_INTERFACE_DEFINED__

/* interface IWOLBrowserScript */
/* [unique][helpstring][uuid][dual][local][object] */ 


EXTERN_C const IID IID_IWOLBrowserScript;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A992E36-766A-4ff9-AF04-8541DC43B2DD")
    IWOLBrowserScript : public IDispatch
    {
    public:
        virtual int32_t STDMETHODCALLTYPE Quit( void) = 0;
        
        virtual /* [propput] */ int32_t STDMETHODCALLTYPE put_ErrorMsg( 
            /* [in] */ BSTR error) = 0;
        
        virtual /* [propget] */ int32_t STDMETHODCALLTYPE get_BackURL( 
            /* [out][retval] */ BSTR __RPC_FAR *url) = 0;
        
        virtual /* [propput] */ int32_t STDMETHODCALLTYPE put_BackURL( 
            /* [in] */ BSTR url) = 0;
        
        virtual int32_t STDMETHODCALLTYPE RegisterLogin( 
            /* [in] */ BSTR nickname,
            /* [in] */ BSTR password) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWOLBrowserScriptVtbl
    {
        BEGIN_INTERFACE
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        uint32_t ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWOLBrowserScript __RPC_FAR * This);
        
        uint32_t ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWOLBrowserScript __RPC_FAR * This);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [out] */ uint32_t __RPC_FAR *pctinfo);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ uint32_t iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ uint32_t cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ int32_t ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ uint16_t wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ uint32_t __RPC_FAR *puArgErr);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *Quit )( 
            IWOLBrowserScript __RPC_FAR * This);
        
        /* [propput] */ int32_t ( STDMETHODCALLTYPE __RPC_FAR *put_ErrorMsg )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ BSTR error);
        
        /* [propget] */ int32_t ( STDMETHODCALLTYPE __RPC_FAR *get_BackURL )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [out][retval] */ BSTR __RPC_FAR *url);
        
        /* [propput] */ int32_t ( STDMETHODCALLTYPE __RPC_FAR *put_BackURL )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ BSTR url);
        
        int32_t ( STDMETHODCALLTYPE __RPC_FAR *RegisterLogin )( 
            IWOLBrowserScript __RPC_FAR * This,
            /* [in] */ BSTR nickname,
            /* [in] */ BSTR password);
        
        END_INTERFACE
    } IWOLBrowserScriptVtbl;

    interface IWOLBrowserScript
    {
        CONST_VTBL struct IWOLBrowserScriptVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWOLBrowserScript_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWOLBrowserScript_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWOLBrowserScript_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWOLBrowserScript_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWOLBrowserScript_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWOLBrowserScript_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWOLBrowserScript_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWOLBrowserScript_Quit(This)	\
    (This)->lpVtbl -> Quit(This)

#define IWOLBrowserScript_put_ErrorMsg(This,error)	\
    (This)->lpVtbl -> put_ErrorMsg(This,error)

#define IWOLBrowserScript_get_BackURL(This,url)	\
    (This)->lpVtbl -> get_BackURL(This,url)

#define IWOLBrowserScript_put_BackURL(This,url)	\
    (This)->lpVtbl -> put_BackURL(This,url)

#define IWOLBrowserScript_RegisterLogin(This,nickname,password)	\
    (This)->lpVtbl -> RegisterLogin(This,nickname,password)

#endif /* COBJMACROS */


#endif 	/* C style interface */



int32_t STDMETHODCALLTYPE IWOLBrowserScript_Quit_Proxy( 
    IWOLBrowserScript __RPC_FAR * This);


void __RPC_STUB IWOLBrowserScript_Quit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


/* [propput] */ int32_t STDMETHODCALLTYPE IWOLBrowserScript_put_ErrorMsg_Proxy( 
    IWOLBrowserScript __RPC_FAR * This,
    /* [in] */ BSTR error);


void __RPC_STUB IWOLBrowserScript_put_ErrorMsg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


/* [propget] */ int32_t STDMETHODCALLTYPE IWOLBrowserScript_get_BackURL_Proxy( 
    IWOLBrowserScript __RPC_FAR * This,
    /* [out][retval] */ BSTR __RPC_FAR *url);


void __RPC_STUB IWOLBrowserScript_get_BackURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


/* [propput] */ int32_t STDMETHODCALLTYPE IWOLBrowserScript_put_BackURL_Proxy( 
    IWOLBrowserScript __RPC_FAR * This,
    /* [in] */ BSTR url);


void __RPC_STUB IWOLBrowserScript_put_BackURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);


int32_t STDMETHODCALLTYPE IWOLBrowserScript_RegisterLogin_Proxy( 
    IWOLBrowserScript __RPC_FAR * This,
    /* [in] */ BSTR nickname,
    /* [in] */ BSTR password);


void __RPC_STUB IWOLBrowserScript_RegisterLogin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    uint32_t *_pdwStubPhase);



#endif 	/* __IWOLBrowserScript_INTERFACE_DEFINED__ */



#ifndef __WOLBrowserLib_LIBRARY_DEFINED__
#define __WOLBrowserLib_LIBRARY_DEFINED__

/* library WOLBrowserLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_WOLBrowserLib;

EXTERN_C const CLSID CLSID_WOLBrowser;

#ifdef __cplusplus

class DECLSPEC_UUID("E6F33E57-C630-4EAF-AA4A-43F3A59EA608")
WOLBrowser;
#endif
#endif /* __WOLBrowserLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
