/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sat Dec 17 13:53:00 2005
 */
/* Compiler settings for isimpledomdocument.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __isimpledomdocument_h__
#define __isimpledomdocument_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISimpleDOMDocument_FWD_DEFINED__
#define __ISimpleDOMDocument_FWD_DEFINED__
typedef interface ISimpleDOMDocument ISimpleDOMDocument;
#endif 	/* __ISimpleDOMDocument_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_isimpledomdocument_0000 */
/* [local] */ 

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ISimpleDOMDocument
//
// @STATUS UNDER_REVIEW
// ---------------------------------------------------------------------------------------------------=
//
// get_URL(out] BSTR *url)
// ---------------------------------------------------------------------------------------------------=
// Get the internet URL associated with this document.
//
// get_title([out BSTR *title
// ---------------------------------------------------------------------------------------------------=
// Get the document's title from the <TITLE> element
//
// get_mimeType([out BSTR *mimeType
// ---------------------------------------------------------------------------------------------------=
// Get the registered mime type, such as text/html
//
// get_docType([out] BSTR *docType
// ---------------------------------------------------------------------------------------------------=
// Get doctype associated with the <!DOCTYPE ..> element
//
// get_nameSpaceURIForID([in] short nameSpaceID, [out] BSTR *nameSpaceURI)
// ---------------------------------------------------------------------------------------------------=
// Some of the methods for ISimpleDOMNode return a nameSpaceID (-1,0,1,2,3,....)
// This method returns the associated namespace URI for each ID.
//
// set_alternateViewMediaTypes([in] BSTR *commaSeparatedMediaType)
// ---------------------------------------------------------------------------------------------------=
// For style property retrieval on nsISimpleDOMNode elements, 
// set the additional alternate media types that properties are available for.
// [in] BSTR *commaSeparatedMediaTypes is a comma separate list, for example "aural, braille".
// The alternate media properties are requested with nsISimpleDOMNode::get_computedStyle.
// Note: setting this value on a document will increase memory overhead, and may create a small delay.
//
// W3C media Types:
// * all:        Suitable for all devices. 
// * aural:      Intended for speech synthesizers. See the section on aural style sheets for details. 
// * braille:    Intended for braille tactile feedback devices. 
// * embossed:   Intended for paged braille printers. 
// * handheld:   Intended for handheld devices - typically small screen, monochrome, limited bandwidth. 
// * print:      Intended for paged, opaque material and for documents viewed on screen in print preview mode. Please consult the section on paged media for information about formatting issues that are specific to paged media. 
// * projection: Intended for projected presentations, for example projectors or print to transparencies. Please consult the section on paged media for information about formatting issues that are specific to paged media. 
// * screen:     Intended primarily for color computer screens. 
// * tty:        intended for media using a fixed-pitch character grid, such as teletypes, terminals, or portable devices with limited display capabilities. Authors should not use pixel units with the tty media type. 
// * tv:         Intended for television-type devices - low resolution, color, limited-scrollability screens, sound
// * See latest W3C CSS specs for complete list of media types
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////


#define	DISPID_DOC_URL	( -5904 )

#define	DISPID_DOC_TITLE	( -5905 )

#define	DISPID_DOC_MIMETYPE	( -5906 )

#define	DISPID_DOC_DOCTYPE	( -5907 )

#define	DISPID_DOC_NAMESPACE	( -5908 )

#define	DISPID_DOC_MEDIATYPES	( -5909 )



extern RPC_IF_HANDLE __MIDL_itf_isimpledomdocument_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_isimpledomdocument_0000_v0_0_s_ifspec;

#ifndef __ISimpleDOMDocument_INTERFACE_DEFINED__
#define __ISimpleDOMDocument_INTERFACE_DEFINED__

/* interface ISimpleDOMDocument */
/* [uuid][object] */ 


EXTERN_C const IID IID_ISimpleDOMDocument;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0D68D6D0-D93D-4d08-A30D-F00DD1F45B24")
    ISimpleDOMDocument : public IUnknown
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
            /* [retval][out] */ BSTR __RPC_FAR *url) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_title( 
            /* [retval][out] */ BSTR __RPC_FAR *title) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mimeType( 
            /* [retval][out] */ BSTR __RPC_FAR *mimeType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_docType( 
            /* [retval][out] */ BSTR __RPC_FAR *docType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_nameSpaceURIForID( 
            /* [in] */ short nameSpaceID,
            /* [retval][out] */ BSTR __RPC_FAR *nameSpaceURI) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_alternateViewMediaTypes( 
            /* [in] */ BSTR __RPC_FAR *commaSeparatedMediaTypes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleDOMDocumentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISimpleDOMDocument __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISimpleDOMDocument __RPC_FAR * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_URL )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *url);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_title )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *title);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_mimeType )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *mimeType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_docType )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *docType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_nameSpaceURIForID )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [in] */ short nameSpaceID,
            /* [retval][out] */ BSTR __RPC_FAR *nameSpaceURI);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_alternateViewMediaTypes )( 
            ISimpleDOMDocument __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *commaSeparatedMediaTypes);
        
        END_INTERFACE
    } ISimpleDOMDocumentVtbl;

    interface ISimpleDOMDocument
    {
        CONST_VTBL struct ISimpleDOMDocumentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleDOMDocument_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleDOMDocument_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleDOMDocument_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleDOMDocument_get_URL(This,url)	\
    (This)->lpVtbl -> get_URL(This,url)

#define ISimpleDOMDocument_get_title(This,title)	\
    (This)->lpVtbl -> get_title(This,title)

#define ISimpleDOMDocument_get_mimeType(This,mimeType)	\
    (This)->lpVtbl -> get_mimeType(This,mimeType)

#define ISimpleDOMDocument_get_docType(This,docType)	\
    (This)->lpVtbl -> get_docType(This,docType)

#define ISimpleDOMDocument_get_nameSpaceURIForID(This,nameSpaceID,nameSpaceURI)	\
    (This)->lpVtbl -> get_nameSpaceURIForID(This,nameSpaceID,nameSpaceURI)

#define ISimpleDOMDocument_put_alternateViewMediaTypes(This,commaSeparatedMediaTypes)	\
    (This)->lpVtbl -> put_alternateViewMediaTypes(This,commaSeparatedMediaTypes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMDocument_get_URL_Proxy( 
    ISimpleDOMDocument __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *url);


void __RPC_STUB ISimpleDOMDocument_get_URL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMDocument_get_title_Proxy( 
    ISimpleDOMDocument __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *title);


void __RPC_STUB ISimpleDOMDocument_get_title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMDocument_get_mimeType_Proxy( 
    ISimpleDOMDocument __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *mimeType);


void __RPC_STUB ISimpleDOMDocument_get_mimeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMDocument_get_docType_Proxy( 
    ISimpleDOMDocument __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *docType);


void __RPC_STUB ISimpleDOMDocument_get_docType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMDocument_get_nameSpaceURIForID_Proxy( 
    ISimpleDOMDocument __RPC_FAR * This,
    /* [in] */ short nameSpaceID,
    /* [retval][out] */ BSTR __RPC_FAR *nameSpaceURI);


void __RPC_STUB ISimpleDOMDocument_get_nameSpaceURIForID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ISimpleDOMDocument_put_alternateViewMediaTypes_Proxy( 
    ISimpleDOMDocument __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *commaSeparatedMediaTypes);


void __RPC_STUB ISimpleDOMDocument_put_alternateViewMediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleDOMDocument_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
