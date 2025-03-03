#pragma once

#include "MRViewerFwd.h"
#include <boost/signals2/signal.hpp>
#include <filesystem>
#include <vector>

namespace MR
{

struct IConnectable
{
    virtual ~IConnectable() = default;
    virtual void connect( Viewer* viewer, int group = 0, boost::signals2::connect_position pos = boost::signals2::connect_position::at_back ) = 0;
    virtual void disconnect() = 0;
};

struct ConnectionHolder : virtual IConnectable
{
    virtual ~ConnectionHolder() = default;
    virtual void disconnect() { connection_.disconnect(); }
protected:
    boost::signals2::connection connection_;
};

template<typename ...Connectables>
struct MultiListener : Connectables...
{
    static_assert( ( std::is_base_of_v<IConnectable, Connectables> && ... ),
        "Base classes must be children of IConnectable" );

    virtual ~MultiListener() = default;

    virtual void connect( Viewer* viewer, int group = 0, boost::signals2::connect_position pos = boost::signals2::connect_position::at_back )
    {
        ( Connectables::connect( viewer, group, pos ), ... );
    }
    virtual void disconnect()
    {
        // disconnect in reversed order
        [[maybe_unused]] int dummy;
        (void)( dummy = ... = ( Connectables::disconnect(), 0 ) );
    }
};

struct MRVIEWER_CLASS MouseDownListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( MouseDownListener );
    virtual ~MouseDownListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onMouseDown_( MouseButton btn, int modifiers ) = 0;
};

struct MRVIEWER_CLASS MouseUpListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( MouseUpListener );
    virtual ~MouseUpListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onMouseUp_( MouseButton btn, int modifiers ) = 0;
};

struct MRVIEWER_CLASS MouseMoveListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( MouseMoveListener );
    virtual ~MouseMoveListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onMouseMove_( int x, int y ) = 0;
};

struct MRVIEWER_CLASS MouseScrollListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( MouseScrollListener );
    virtual ~MouseScrollListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onMouseScroll_( float delta ) = 0;
};

struct MRVIEWER_CLASS CharPressedListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( CharPressedListener );
    virtual ~CharPressedListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onCharPressed_( unsigned charKey, int modifier ) = 0;
};

struct MRVIEWER_CLASS KeyUpListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( KeyUpListener );
    virtual ~KeyUpListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onKeyUp_( int key, int modifier ) = 0;
};

struct MRVIEWER_CLASS KeyDownListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( KeyDownListener );
    virtual ~KeyDownListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onKeyDown_( int key, int modifier ) = 0;
};

struct MRVIEWER_CLASS KeyRepeatListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( KeyRepeatListener );
    virtual ~KeyRepeatListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool onKeyRepeat_( int key, int modifier ) = 0;
};

struct MRVIEWER_CLASS PreDrawListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( PreDrawListener );
    virtual ~PreDrawListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual void preDraw_() = 0;
};

struct MRVIEWER_CLASS DrawListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( DrawListener );
    virtual ~DrawListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual void draw_() = 0;
};

struct MRVIEWER_CLASS PostDrawListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( PostDrawListener );
    virtual ~PostDrawListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual void postDraw_() = 0;
};

struct MRVIEWER_CLASS DragDropListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( DragDropListener );
    virtual ~DragDropListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool dragDrop_( const std::vector<std::filesystem::path>& paths ) = 0;
};

struct MRVIEWER_CLASS PostResizeListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( PostResizeListener );
    virtual ~PostResizeListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual void postResize_( int w, int h ) = 0;
};

struct MRVIEWER_CLASS InterruptCloseListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( InterruptCloseListener );
    virtual ~InterruptCloseListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual bool interruptClose_() = 0;
};

struct MRVIEWER_CLASS PostRescaleListener : ConnectionHolder
{
    MR_ADD_CTOR_DELETE_MOVE( PostRescaleListener );
    virtual ~PostRescaleListener() = default;
    MRVIEWER_API virtual void connect( Viewer* viewer, int group, boost::signals2::connect_position pos ) override;
protected:
    virtual void postRescale_( float x, float y ) = 0;
};

}
