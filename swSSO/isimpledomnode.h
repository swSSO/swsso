/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sat Dec 17 13:52:51 2005
 */
/* Compiler settings for isimpledomnode.idl:
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

#ifndef __isimpledomnode_h__
#define __isimpledomnode_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISimpleDOMNode_FWD_DEFINED__
#define __ISimpleDOMNode_FWD_DEFINED__
typedef interface ISimpleDOMNode ISimpleDOMNode;
#endif 	/* __ISimpleDOMNode_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_isimpledomnode_0000 */
/* [local] */ 

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ISimpleDOMNode
// ---------------------------------------------------------------------------------------------------=
// An interface that extends MSAA's IAccessible to provide readonly DOM node information via cross-process COM.
//
// @STATUS UNDER_REVIEW
//
// get_nodeInfo(
//  /* [out] */ BSTR  *nodeName,   // For elements, this is the tag name
//  /* [out] */ short  *nameSpaceID,
//  /* [out] */ BSTR  *nodeValue, 
//  /* [out] */ unsigned int    *numChildren); 
//  /* [out] */ unsigned int    *uniqueID;  // In Win32 accessible events we generate, the target's childID matches to this
//  /* [out] */ unsigned short  *nodeType,
// ---------------------------------------------------------------------------------------------------=
// Get the basic information about a node.
// The namespace ID can be mapped to an URI using nsISimpleDOMDocument::get_nameSpaceURIForID()
//
// get_attributes(
//  /* [in]  */ unsigned short maxAttribs,
//  /* [out] */ unsigned short  *numAttribs,
//  /* [out] */ BSTR  *attribNames,
//  /* [out] */ short *nameSpaceID,
//  /* [out] */ BSTR  *attribValues);
// ---------------------------------------------------------------------------------------------------=
// Returns 3 arrays - the attribute names and values, and a namespace ID for each
// If the namespace ID is 0, it's the same namespace as the node's namespace
//
// get_attributesForNames(
//  /* [in] */ unsigned short numAttribs,
//  /* [in] */ BSTR   *attribNames,
//  /* [in] */ short  *nameSpaceID,
//  /* [out] */ BSTR  *attribValues);
// ---------------------------------------------------------------------------------------------------=
// Takes 2 arrays - the attribute names and namespace IDs, and returns an array of corresponding values
// If the namespace ID is 0, it's the same namespace as the node's namespace
//
// computedStyle(  
//  /* [in]  */ unsigned short maxStyleProperties,
//  /* [out] */ unsigned short *numStyleProperties, 
//  /* [in]  */ boolean useAlternateView,  // If TRUE, returns properites for media as set in nsIDOMDocument::set_alternateViewMediaTypes
//  /* [out] */ BSTR *styleProperties, 
//  /* [out] */ BSTR *styleValues);
// ---------------------------------------------------------------------------------------------------=
// Returns 2 arrays -- the style properties and their values
//  useAlternateView=FALSE: gets properties for the default media type (usually screen)
//  useAlternateView=TRUE: properties for media types set w/ nsIDOMSimpleDocument::set_alternateViewMediaTypes()
//
// computedStyleForProperties(  
//  /* [in] */  unsigned short numStyleProperties, 
//  /* [in] */  boolean useAlternateView,  // If TRUE, returns properites for media as set in nsIDOMDocument::set_alternateViewMediaTypes
//  /* [in] */  BSTR *styleProperties, 
//  /* [out] */ BSTR *styleValues);
// ---------------------------------------------------------------------------------------------------=
// Scroll the current view so that this dom node is visible.
//  placeTopLeft=TRUE: scroll until the top left corner of the dom node is at the top left corner of the view.
//  placeTopLeft=FALSE: scroll minimally to make the dom node visible. Don't scroll at all if already visible.
//
// scrollTo( 
//  /* [in] */ boolean placeTopLeft); 
// ---------------------------------------------------------------------------------------------------=
// Returns style property values for those properties in the styleProperties [in] array
// Returns 2 arrays -- the style properties and their values
//  useAlternateView=FALSE: gets properties for the default media type (usually screen)
//  useAlternateView=TRUE: properties for media types set w/ nsIDOMSimpleDocument::set_alternateViewMediaTypes()
//
// get_parentNode     (/* [out] */ ISimpleDOMNode **newNodePtr);
// get_firstChild     (/* [out] */ ISimpleDOMNode **newNodePtr);
// get_lastChild      (/* [out] */ ISimpleDOMNode **newNodePtr);
// get_previousSibling(/* [out] */ ISimpleDOMNode **newNodePtr);
// get_nextSibling    (/* [out] */ ISimpleDOMNode **newNodePtr);
// get_childAt        (/* [in] */ unsigned childIndex, /* [out] */ ISimpleDOMNode **newNodePtr);
// ---------------------------------------------------------------------------------------------------=
// DOM navigation - get a different node.
//
// get_innerHTML(/* [out] */ BSTR *htmlText);
// ---------------------------------------------------------------------------------------------------=
// Returns HTML of this DOM node's subtree. Does not include the start and end tag for this node/element.
//
//
// get_localInterface(/* [out] */ void **localInterface);
// ---------------------------------------------------------------------------------------------------=
// Only available in Gecko's process - casts to an XPCOM nsIAccessNode interface pointer
//
//
// get_language(/* [out] */ BSTR *htmlText);
// ---------------------------------------------------------------------------------------------------=
// Returns the computed language for this node, or empty string if unknown.
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////


#define	DISPID_NODE_NODEINFO	( -5900 )

#define	DISPID_NODE_ATTRIBUTES	( -5901 )

#define	DISPID_NODE_ATTRIBUTESFORNAMES	( -5902 )

#define	DISPID_NODE_COMPSTYLE	( -5903 )

#define	DISPID_NODE_COMPSTYLEFORPROPS	( -5904 )

#define	DISPID_NODE_LANGUAGE	( -5905 )



extern RPC_IF_HANDLE __MIDL_itf_isimpledomnode_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_isimpledomnode_0000_v0_0_s_ifspec;

#ifndef __ISimpleDOMNode_INTERFACE_DEFINED__
#define __ISimpleDOMNode_INTERFACE_DEFINED__

/* interface ISimpleDOMNode */
/* [uuid][object] */ 

#define	NODETYPE_ELEMENT	( 1 )

#define	NODETYPE_ATTRIBUTE	( 2 )

#define	NODETYPE_TEXT	( 3 )

#define	NODETYPE_CDATA_SECTION	( 4 )

#define	NODETYPE_ENTITY_REFERENCE	( 5 )

#define	NODETYPE_ENTITY	( 6 )

#define	NODETYPE_PROCESSING_INSTRUCTION	( 7 )

#define	NODETYPE_COMMENT	( 8 )

#define	NODETYPE_DOCUMENT	( 9 )

#define	NODETYPE_DOCUMENT_TYPE	( 10 )

#define	NODETYPE_DOCUMENT_FRAGMENT	( 11 )

#define	NODETYPE_NOTATION	( 12 )


EXTERN_C const IID IID_ISimpleDOMNode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1814ceeb-49e2-407f-af99-fa755a7d2607")
    ISimpleDOMNode : public IUnknown
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeInfo( 
            /* [out] */ BSTR __RPC_FAR *nodeName,
            /* [out] */ short __RPC_FAR *nameSpaceID,
            /* [out] */ BSTR __RPC_FAR *nodeValue,
            /* [out] */ unsigned int __RPC_FAR *numChildren,
            /* [out] */ unsigned int __RPC_FAR *uniqueID,
            /* [retval][out] */ unsigned short __RPC_FAR *nodeType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_attributes( 
            /* [in] */ unsigned short maxAttribs,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribNames,
            /* [length_is][size_is][out] */ short __RPC_FAR *nameSpaceID,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribValues,
            /* [retval][out] */ unsigned short __RPC_FAR *numAttribs) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_attributesForNames( 
            /* [in] */ unsigned short numAttribs,
            /* [length_is][size_is][in] */ BSTR __RPC_FAR *attribNames,
            /* [length_is][size_is][in] */ short __RPC_FAR *nameSpaceID,
            /* [length_is][size_is][retval][out] */ BSTR __RPC_FAR *attribValues) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_computedStyle( 
            /* [in] */ unsigned short maxStyleProperties,
            /* [in] */ boolean useAlternateView,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleProperties,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleValues,
            /* [retval][out] */ unsigned short __RPC_FAR *numStyleProperties) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_computedStyleForProperties( 
            /* [in] */ unsigned short numStyleProperties,
            /* [in] */ boolean useAlternateView,
            /* [length_is][size_is][in] */ BSTR __RPC_FAR *styleProperties,
            /* [length_is][size_is][retval][out] */ BSTR __RPC_FAR *styleValues) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE scrollTo( 
            /* [in] */ boolean placeTopLeft) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_firstChild( 
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_lastChild( 
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling( 
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling( 
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_childAt( 
            /* [in] */ unsigned int childIndex,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_innerHTML( 
            /* [retval][out] */ BSTR __RPC_FAR *innerHTML) = 0;
        
        virtual /* [local][propget] */ HRESULT STDMETHODCALLTYPE get_localInterface( 
            /* [retval][out] */ void __RPC_FAR *__RPC_FAR *localInterface) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_language( 
            /* [retval][out] */ BSTR __RPC_FAR *language) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleDOMNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISimpleDOMNode __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISimpleDOMNode __RPC_FAR * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_nodeInfo )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *nodeName,
            /* [out] */ short __RPC_FAR *nameSpaceID,
            /* [out] */ BSTR __RPC_FAR *nodeValue,
            /* [out] */ unsigned int __RPC_FAR *numChildren,
            /* [out] */ unsigned int __RPC_FAR *uniqueID,
            /* [retval][out] */ unsigned short __RPC_FAR *nodeType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_attributes )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ unsigned short maxAttribs,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribNames,
            /* [length_is][size_is][out] */ short __RPC_FAR *nameSpaceID,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribValues,
            /* [retval][out] */ unsigned short __RPC_FAR *numAttribs);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_attributesForNames )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ unsigned short numAttribs,
            /* [length_is][size_is][in] */ BSTR __RPC_FAR *attribNames,
            /* [length_is][size_is][in] */ short __RPC_FAR *nameSpaceID,
            /* [length_is][size_is][retval][out] */ BSTR __RPC_FAR *attribValues);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_computedStyle )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ unsigned short maxStyleProperties,
            /* [in] */ boolean useAlternateView,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleProperties,
            /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleValues,
            /* [retval][out] */ unsigned short __RPC_FAR *numStyleProperties);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_computedStyleForProperties )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ unsigned short numStyleProperties,
            /* [in] */ boolean useAlternateView,
            /* [length_is][size_is][in] */ BSTR __RPC_FAR *styleProperties,
            /* [length_is][size_is][retval][out] */ BSTR __RPC_FAR *styleValues);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *scrollTo )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ boolean placeTopLeft);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_parentNode )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_firstChild )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_lastChild )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_previousSibling )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_nextSibling )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_childAt )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [in] */ unsigned int childIndex,
            /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_innerHTML )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *innerHTML);
        
        /* [local][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_localInterface )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ void __RPC_FAR *__RPC_FAR *localInterface);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_language )( 
            ISimpleDOMNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *language);
        
        END_INTERFACE
    } ISimpleDOMNodeVtbl;

    interface ISimpleDOMNode
    {
        CONST_VTBL struct ISimpleDOMNodeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleDOMNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleDOMNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleDOMNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleDOMNode_get_nodeInfo(This,nodeName,nameSpaceID,nodeValue,numChildren,uniqueID,nodeType)	\
    (This)->lpVtbl -> get_nodeInfo(This,nodeName,nameSpaceID,nodeValue,numChildren,uniqueID,nodeType)

#define ISimpleDOMNode_get_attributes(This,maxAttribs,attribNames,nameSpaceID,attribValues,numAttribs)	\
    (This)->lpVtbl -> get_attributes(This,maxAttribs,attribNames,nameSpaceID,attribValues,numAttribs)

#define ISimpleDOMNode_get_attributesForNames(This,numAttribs,attribNames,nameSpaceID,attribValues)	\
    (This)->lpVtbl -> get_attributesForNames(This,numAttribs,attribNames,nameSpaceID,attribValues)

#define ISimpleDOMNode_get_computedStyle(This,maxStyleProperties,useAlternateView,styleProperties,styleValues,numStyleProperties)	\
    (This)->lpVtbl -> get_computedStyle(This,maxStyleProperties,useAlternateView,styleProperties,styleValues,numStyleProperties)

#define ISimpleDOMNode_get_computedStyleForProperties(This,numStyleProperties,useAlternateView,styleProperties,styleValues)	\
    (This)->lpVtbl -> get_computedStyleForProperties(This,numStyleProperties,useAlternateView,styleProperties,styleValues)

#define ISimpleDOMNode_scrollTo(This,placeTopLeft)	\
    (This)->lpVtbl -> scrollTo(This,placeTopLeft)

#define ISimpleDOMNode_get_parentNode(This,node)	\
    (This)->lpVtbl -> get_parentNode(This,node)

#define ISimpleDOMNode_get_firstChild(This,node)	\
    (This)->lpVtbl -> get_firstChild(This,node)

#define ISimpleDOMNode_get_lastChild(This,node)	\
    (This)->lpVtbl -> get_lastChild(This,node)

#define ISimpleDOMNode_get_previousSibling(This,node)	\
    (This)->lpVtbl -> get_previousSibling(This,node)

#define ISimpleDOMNode_get_nextSibling(This,node)	\
    (This)->lpVtbl -> get_nextSibling(This,node)

#define ISimpleDOMNode_get_childAt(This,childIndex,node)	\
    (This)->lpVtbl -> get_childAt(This,childIndex,node)

#define ISimpleDOMNode_get_innerHTML(This,innerHTML)	\
    (This)->lpVtbl -> get_innerHTML(This,innerHTML)

#define ISimpleDOMNode_get_localInterface(This,localInterface)	\
    (This)->lpVtbl -> get_localInterface(This,localInterface)

#define ISimpleDOMNode_get_language(This,language)	\
    (This)->lpVtbl -> get_language(This,language)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_nodeInfo_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *nodeName,
    /* [out] */ short __RPC_FAR *nameSpaceID,
    /* [out] */ BSTR __RPC_FAR *nodeValue,
    /* [out] */ unsigned int __RPC_FAR *numChildren,
    /* [out] */ unsigned int __RPC_FAR *uniqueID,
    /* [retval][out] */ unsigned short __RPC_FAR *nodeType);


void __RPC_STUB ISimpleDOMNode_get_nodeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_attributes_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [in] */ unsigned short maxAttribs,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribNames,
    /* [length_is][size_is][out] */ short __RPC_FAR *nameSpaceID,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *attribValues,
    /* [retval][out] */ unsigned short __RPC_FAR *numAttribs);


void __RPC_STUB ISimpleDOMNode_get_attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_attributesForNames_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [in] */ unsigned short numAttribs,
    /* [length_is][size_is][in] */ BSTR __RPC_FAR *attribNames,
    /* [length_is][size_is][in] */ short __RPC_FAR *nameSpaceID,
    /* [length_is][size_is][retval][out] */ BSTR __RPC_FAR *attribValues);


void __RPC_STUB ISimpleDOMNode_get_attributesForNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_computedStyle_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [in] */ unsigned short maxStyleProperties,
    /* [in] */ boolean useAlternateView,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleProperties,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *styleValues,
    /* [retval][out] */ unsigned short __RPC_FAR *numStyleProperties);


void __RPC_STUB ISimpleDOMNode_get_computedStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_computedStyleForProperties_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [in] */ unsigned short numStyleProperties,
    /* [in] */ boolean useAlternateView,
    /* [length_is][size_is][in] */ BSTR __RPC_FAR *styleProperties,
    /* [length_is][size_is][retval][out] */ BSTR __RPC_FAR *styleValues);


void __RPC_STUB ISimpleDOMNode_get_computedStyleForProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleDOMNode_scrollTo_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [in] */ boolean placeTopLeft);


void __RPC_STUB ISimpleDOMNode_scrollTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_parentNode_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);


void __RPC_STUB ISimpleDOMNode_get_parentNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_firstChild_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);


void __RPC_STUB ISimpleDOMNode_get_firstChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_lastChild_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);


void __RPC_STUB ISimpleDOMNode_get_lastChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_previousSibling_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);


void __RPC_STUB ISimpleDOMNode_get_previousSibling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_nextSibling_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);


void __RPC_STUB ISimpleDOMNode_get_nextSibling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_childAt_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [in] */ unsigned int childIndex,
    /* [retval][out] */ ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);


void __RPC_STUB ISimpleDOMNode_get_childAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_innerHTML_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *innerHTML);


void __RPC_STUB ISimpleDOMNode_get_innerHTML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_localInterface_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ void __RPC_FAR *__RPC_FAR *localInterface);


void __RPC_STUB ISimpleDOMNode_get_localInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ISimpleDOMNode_get_language_Proxy( 
    ISimpleDOMNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *language);


void __RPC_STUB ISimpleDOMNode_get_language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleDOMNode_INTERFACE_DEFINED__ */


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
