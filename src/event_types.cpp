#include "event_types.h"
#include <cstring>

struct event_info_t {
    char const *name;
    int e;
    char const *desc;
};

#define MAXN 500
static struct event_info_t events_infos[MAXN];
static int N = 0;

static void add_et(char const *const name, int e, char const *const desc)
{
    events_infos[N].name = name;
    events_infos[N].e = e;
    events_infos[N].desc = desc;
    N++;
}

static int e_to_N(const int e)
{
    for(int i = 0; i < MAXN; i++) {
        if(events_infos[i].e == e) {
            return i;
        }
    }

    return (-1);
}

static bool isinit = false;
static void init()
{
    if(isinit) {
        return;
    }

    isinit = true;

    for(int i = 0; i < MAXN; i++) {
        events_infos[N].name = NULL;
        events_infos[N].e = -1;
        events_infos[N].desc = NULL;
    }

    add_et("None", 0, "Not an event.");
    add_et("ActionAdded", 114, "A new action has been added (QActionEvent).");
    add_et("ActionChanged", 113, "An action has been changed (QActionEvent).");
    add_et("ActionRemoved", 115, "An action has been removed (QActionEvent).");
    add_et("ActivationChange", 99, "A widget's top-level window activation state has changed.");
    add_et("ApplicationActivate", 121, "This enum has been deprecated. Use ApplicationStateChange instead.");
    add_et("ApplicationDeactivate", 122, "This enum has been deprecated. Use ApplicationStateChange instead.");
    add_et("ApplicationFontChange", 36, "The default application font has changed.");
    add_et("ApplicationLayoutDirectionChange", 37, "The default application layout direction has changed.");
    add_et("ApplicationPaletteChange", 38, "The default application palette has changed.");
    add_et("ApplicationStateChange", 214, "The state of the application has changed.");
    add_et("ApplicationWindowIconChange", 35, "The application's icon has changed.");
    add_et("ChildAdded", 68, "An object gets a child (QChildEvent).");
    add_et("ChildPolished", 69, "A widget child gets polished (QChildEvent).");
    add_et("ChildRemoved", 71, "An object loses a child (QChildEvent).");
    add_et("Clipboard", 40, "The clipboard contents have changed.");
    add_et("Close", 19, "Widget was closed (QCloseEvent).");
    add_et("CloseSoftwareInputPanel", 200, "A widget wants to close the software input panel (SIP).");
    add_et("ContentsRectChange", 178, "The margins of the widget's content rect changed.");
    add_et("ContextMenu", 82, "Context popup menu (QContextMenuEvent).");
    add_et("CursorChange", 183, "The widget's cursor has changed.");
    add_et("DeferredDelete", 52, "The object will be deleted after it has cleaned up (QDeferredDeleteEvent)");
    add_et("DragEnter", 60, "The cursor enters a widget during a drag and drop operation (QDragEnterEvent).");
    add_et("DragLeave", 62, "The cursor leaves a widget during a drag and drop operation (QDragLeaveEvent).");
    add_et("DragMove", 61, "A drag and drop operation is in progress (QDragMoveEvent).");
    add_et("Drop", 63, "A drag and drop operation is completed (QDropEvent).");
    add_et("DynamicPropertyChange", 170, "A dynamic property was added, changed, or removed from the object.");
    add_et("EnabledChange", 98, "Widget's enabled state has changed.");
    add_et("Enter", 10, "Mouse enters widget's boundaries (QEnterEvent).");
    add_et("EnterEditFocus", 150, "An editor widget gains focus for editing. QT_KEYPAD_NAVIGATION must be defined.");
    add_et("EnterWhatsThisMode", 124, "Send to toplevel widgets when the application enters WhatsThis mode.");
    add_et("Expose", 206, "Sent to a window when its on-screen contents are invalidated and need to be flushed from the backing store.");
    add_et("FileOpen", 116, "File open request (QFileOpenEvent).");
    add_et("FocusIn", 8, "Widget or Window gains keyboard focus (QFocusEvent).");
    add_et("FocusOut", 9, "Widget or Window loses keyboard focus (QFocusEvent).");
    add_et("FocusAboutToChange", 23, "Widget or Window focus is about to change (QFocusEvent)");
    add_et("FontChange", 97, "Widget's font has changed.");
    add_et("Gesture", 198, "A gesture was triggered(QGestureEvent).");
    add_et("GestureOverride", 202, "A gesture override was triggered(QGestureEvent).");
    add_et("GrabKeyboard", 188, "Item gains keyboard grab(QGraphicsItem only).");
    add_et("GrabMouse", 186, "Item gains mouse grab(QGraphicsItem only).");
    add_et("GraphicsSceneContextMenu", 159, "Context popup menu over a graphics scene(QGraphicsSceneContextMenuEvent).");
    add_et("GraphicsSceneDragEnter", 164, "The cursor enters a graphics scene during a drag and drop operation(QGraphicsSceneDragDropEvent).");
    add_et("GraphicsSceneDragLeave", 166, "The cursor leaves a graphics scene during a drag and drop operation(QGraphicsSceneDragDropEvent).");
    add_et("GraphicsSceneDragMove", 165, "A drag and drop operation is in progress over a scene(QGraphicsSceneDragDropEvent).");
    add_et("GraphicsSceneDrop", 167, "A drag and drop operation is completed over a scene(QGraphicsSceneDragDropEvent).");
    add_et("GraphicsSceneHelp", 163, "The user requests help for a graphics scene(QHelpEvent).");
    add_et("GraphicsSceneHoverEnter", 160, "The mouse cursor enters a hover item in a graphics scene(QGraphicsSceneHoverEvent).");
    add_et("GraphicsSceneHoverLeave", 162, "The mouse cursor leaves a hover item in a graphics scene(QGraphicsSceneHoverEvent).");
    add_et("GraphicsSceneHoverMove", 161, "The mouse cursor moves inside a hover item in a graphics scene(QGraphicsSceneHoverEvent).");
    add_et("GraphicsSceneMouseDoubleClick", 158, "Mouse press again(double click) in a graphics scene(QGraphicsSceneMouseEvent).");
    add_et("GraphicsSceneMouseMove", 155, "Move mouse in a graphics scene(QGraphicsSceneMouseEvent).");
    add_et("GraphicsSceneMousePress", 156, "Mouse press in a graphics scene(QGraphicsSceneMouseEvent).");
    add_et("GraphicsSceneMouseRelease", 157, "Mouse release in a graphics scene(QGraphicsSceneMouseEvent).");
    add_et("GraphicsSceneMove", 182, "Widget was moved(QGraphicsSceneMoveEvent).");
    add_et("GraphicsSceneResize", 181, "Widget was resized(QGraphicsSceneResizeEvent).");
    add_et("GraphicsSceneWheel", 168, "Mouse wheel rolled in a graphics scene(QGraphicsSceneWheelEvent).");
    add_et("Hide", 18, "Widget was hidden(QHideEvent).");
    add_et("HideToParent", 27, "A child widget has been hidden.");
    add_et("HoverEnter", 127, "The mouse cursor enters a hover widget(QHoverEvent).");
    add_et("HoverLeave", 128, "The mouse cursor leaves a hover widget(QHoverEvent).");
    add_et("HoverMove", 129, "The mouse cursor moves inside a hover widget(QHoverEvent).");
    add_et("IconDrag", 96, "The main icon of a window has been dragged away(QIconDragEvent).");
    add_et("IconTextChange", 101, "Widget's icon text has been changed.");
    add_et("InputMethod", 83, "An input method is being used (QInputMethodEvent).");
    add_et("InputMethodQuery", 207, "A input method query event (QInputMethodQueryEvent)");
    add_et("KeyboardLayoutChange", 169, "The keyboard layout has changed.");
    add_et("KeyPress", 6, "Key press (QKeyEvent).");
    add_et("KeyRelease", 7, "Key release (QKeyEvent).");
    add_et("LanguageChange", 89, "The application translation changed.");
    add_et("LayoutDirectionChange", 90, "The direction of layouts changed.");
    add_et("LayoutRequest", 76, "Widget layout needs to be redone.");
    add_et("Leave", 11, "Mouse leaves widget's boundaries.");
    add_et("LeaveEditFocus", 151, "An editor widget loses focus for editing. QT_KEYPAD_NAVIGATION must be defined.");
    add_et("LeaveWhatsThisMode", 125, "Send to toplevel widgets when the application leaves WhatsThis mode.");
    add_et("LocaleChange", 88, "The system locale has changed.");
    add_et("NonClientAreaMouseButtonDblClick", 176, "A mouse double click occurred outside the client area.");
    add_et("NonClientAreaMouseButtonPress", 174, "A mouse button press occurred outside the client area.");
    add_et("NonClientAreaMouseButtonRelease", 175, "A mouse button release occurred outside the client area.");
    add_et("NonClientAreaMouseMove", 173, "A mouse move occurred outside the client area.");
    add_et("MacSizeChange", 177, "The user changed his widget sizes(OS X only).");
    add_et("MetaCall", 43, "An asynchronous method invocation via QMetaObject::invokeMethod().");
    add_et("ModifiedChange", 102, "Widgets modification state has been changed.");
    add_et("MouseButtonDblClick", 4, "Mouse press again(QMouseEvent).");
    add_et("MouseButtonPress", 2, "Mouse press(QMouseEvent).");
    add_et("MouseButtonRelease", 3, "Mouse release(QMouseEvent).");
    add_et("MouseMove", 5, "Mouse move(QMouseEvent).");
    add_et("MouseTrackingChange", 109, "The mouse tracking state has changed.");
    add_et("Move", 13, "Widget's position changed(QMoveEvent).");
    add_et("NativeGesture", 197, "The system has detected a gesture(QNativeGestureEvent).");
    add_et("OrientationChange", 208, "The screens orientation has changes(QScreenOrientationChangeEvent).");
    add_et("Paint", 12, "Screen update necessary(QPaintEvent).");
    add_et("PaletteChange", 39, "Palette of the widget changed.");
    add_et("ParentAboutToChange", 131, "The widget parent is about to change.");
    add_et("ParentChange", 21, "The widget parent has changed.");
    add_et("PlatformPanel", 212, "A platform specific panel has been requested.");
    add_et("PlatformSurface", 217, "A native platform surface has been created or is about to be destroyed(QPlatformSurfaceEvent).");
    add_et("Polish", 75, "The widget is polished.");
    add_et("PolishRequest", 74, "The widget should be polished.");
    add_et("QueryWhatsThis", 123, "The widget should accept the event if it has WhatsThis help.");
    add_et("ReadOnlyChange", 106, "Widget's read - only state has changed(since Qt 5.4).");
    add_et("RequestSoftwareInputPanel", 199, "A widget wants to open a software input panel(SIP).");
    add_et("Resize", 14, "Widget's size changed(QResizeEvent).");
    add_et("ScrollPrepare", 204, "The object needs to fill in its geometry information(QScrollPrepareEvent).");
    add_et("Scroll", 205, "The object needs to scroll to the supplied position(QScrollEvent).");
    add_et("Shortcut", 117, "Key press in child for shortcut key handling(QShortcutEvent).");
    add_et("ShortcutOverride", 51, "Key press in child, for overriding shortcut key handling(QKeyEvent).");
    add_et("Show", 17, "Widget was shown on screen(QShowEvent).");
    add_et("ShowToParent", 26, "A child widget has been shown.");
    add_et("SockAct", 50, "Socket activated, used to implement QSocketNotifier.");
    add_et("StateMachineSignal", 192, "A signal delivered to a state machine(QStateMachine::SignalEvent).");
    add_et("StateMachineWrapped", 193, "The event is a wrapper for, i.e., contains, another event(QStateMachine::WrappedEvent).");
    add_et("StatusTip", 112, "A status tip is requested(QStatusTipEvent).");
    add_et("StyleChange", 100, "Widget's style has been changed.");
    add_et("TabletMove", 87, "Wacom tablet move(QTabletEvent).");
    add_et("TabletPress", 92, "Wacom tablet press(QTabletEvent).");
    add_et("TabletRelease", 93, "Wacom tablet release(QTabletEvent).");
    add_et("OkRequest", 94, "Ok button in decoration pressed. Supported only for Windows CE.");
    add_et("TabletEnterProximity", 171, "Wacom tablet enter proximity event(QTabletEvent), sent to QApplication.");
    add_et("TabletLeaveProximity", 172, "Wacom tablet leave proximity event(QTabletEvent), sent to QApplication.");
    add_et("ThreadChange", 22, "The object is moved to another thread. This is the last event sent to this object in the previous thread. See QObject::moveToThread().");
    add_et("Timer", 1, "Regular timer events(QTimerEvent).");
    add_et("ToolBarChange", 120, "The toolbar button is toggled on OS X.");
    add_et("ToolTip", 110, "A tooltip was requested(QHelpEvent).");
    add_et("ToolTipChange", 184, "The widget's tooltip has changed.");
    add_et("TouchBegin", 194, "Beginning of a sequence of touch - screen or track - pad events(QTouchEvent).");
    add_et("TouchCancel", 209, "Cancellation of touch - event sequence(QTouchEvent).");
    add_et("TouchEnd", 196, "End of touch - event sequence(QTouchEvent).");
    add_et("TouchUpdate", 195, "Touch - screen event(QTouchEvent).");
    add_et("UngrabKeyboard", 189, "Item loses keyboard grab(QGraphicsItem only).");
    add_et("UngrabMouse", 187, "Item loses mouse grab(QGraphicsItem only).");
    add_et("UpdateLater", 78, "The widget should be queued to be repainted at a later time.");
    add_et("UpdateRequest", 77, "The widget should be repainted.");
    add_et("WhatsThis", 111, "The widget should reveal WhatsThis help(QHelpEvent).");
    add_et("WhatsThisClicked", 118, "A link in a widget's WhatsThis help was clicked.");
    add_et("Wheel", 31, "Mouse wheel rolled(QWheelEvent).");
    add_et("WinEventAct", 132, "A Windows - specific activation event has occurred.");
    add_et("WindowActivate", 24, "Window was activated.");
    add_et("WindowBlocked", 103, "The window is blocked by a modal dialog.");
    add_et("WindowDeactivate", 25, "Window was deactivated.");
    add_et("WindowIconChange", 34, "The window's icon has changed.");
    add_et("WindowStateChange", 105, "The window's state(minimized, maximized or full - screen) has changed(QWindowStateChangeEvent).");
    add_et("WindowTitleChange", 33, "The window title has changed.");
    add_et("WindowUnblocked", 104, "The window is unblocked after a modal dialog exited.");
    add_et("WinIdChange", 203, "The window system identifer for this native widget has changed.");
    add_et("ZOrderChange", 126, "The widget's z - order has changed. This event is never sent to top level windows.");
}

char const *event_type_2_name(int e)
{
    init();
    int n = e_to_N(e);

    if(e < 0) {
        return NULL;
    }

    return events_infos[n].name;
}
char const *event_type_2_desc(int e)
{
    init();
    int n = e_to_N(e);

    if(e < 0) {
        return NULL;
    }

    return events_infos[n].desc;
}
