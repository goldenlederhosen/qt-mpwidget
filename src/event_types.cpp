#include "event_types.h"
#include "util.h"
#include <cstring>

struct event_type_info_st {
    char const *const name;
    char const *const desc;
};

typedef struct event_type_info_st event_type_info_t;

#define MAXN 218
static_var const event_type_info_t events_type_infos[MAXN] = {
    {"None", "Not an event"}, // 0
    {"Timer", "Regular timer events(QTimerEvent)"}, // 1
    {"MouseButtonPress", "Mouse press(QMouseEvent)"}, // 2
    {"MouseButtonRelease", "Mouse release(QMouseEvent)"}, // 3
    {"MouseButtonDblClick", "Mouse press again(QMouseEvent)"}, // 4
    {"MouseMove", "Mouse move(QMouseEvent)"}, // 5
    {"KeyPress", "Key press (QKeyEvent)"}, // 6
    {"KeyRelease", "Key release (QKeyEvent)"}, // 7
    {"FocusIn", "Widget or Window gains keyboard focus (QFocusEvent)"}, // 8
    {"FocusOut", "Widget or Window loses keyboard focus (QFocusEvent)"}, // 9
    {"Enter", "Mouse enters widget's boundaries (QEnterEvent)"}, // 10
    {"Leave", "Mouse leaves widget's boundaries"}, // 11
    {"Paint", "Screen update necessary(QPaintEvent)"}, // 12
    {"Move", "Widget's position changed(QMoveEvent)"}, // 13
    {"Resize", "Widget's size changed(QResizeEvent)"}, // 14
    {"UnknownEvent15", "UnknownEvent15"}, // 15
    {"UnknownEvent16", "UnknownEvent16"}, // 16
    {"Show", "Widget was shown on screen(QShowEvent)"}, // 17
    {"Hide", "Widget was hidden(QHideEvent)"}, // 18
    {"Close", "Widget was closed (QCloseEvent)"}, // 19
    {"UnknownEvent20", "UnknownEvent20"}, // 20
    {"ParentChange", "The widget parent has changed"}, // 21
    {"ThreadChange", "The object is moved to another thread. This is the last event sent to this object in the previous thread. See QObject::moveToThread()"}, // 22
    {"FocusAboutToChange", "Widget or Window focus is about to change (QFocusEvent)"}, // 23
    {"WindowActivate", "Window was activated"}, // 24
    {"WindowDeactivate", "Window was deactivated"}, // 25
    {"ShowToParent", "A child widget has been shown"}, // 26
    {"HideToParent", "A child widget has been hidden"}, // 27
    {"UnknownEvent28", "UnknownEvent28"}, // 28
    {"UnknownEvent29", "UnknownEvent29"}, // 29
    {"UnknownEvent30", "UnknownEvent30"}, // 30
    {"Wheel", "Mouse wheel rolled(QWheelEvent)"}, // 31
    {"UnknownEvent32", "UnknownEvent32"}, // 32
    {"WindowTitleChange", "The window title has changed"}, // 33
    {"WindowIconChange", "The window's icon has changed"}, // 34
    {"ApplicationWindowIconChange", "The application's icon has changed"}, // 35
    {"ApplicationFontChange", "The default application font has changed"}, // 36
    {"ApplicationLayoutDirectionChange", "The default application layout direction has changed"}, // 37
    {"ApplicationPaletteChange", "The default application palette has changed"}, // 38
    {"PaletteChange", "Palette of the widget changed"}, // 39
    {"Clipboard", "The clipboard contents have changed"}, // 40
    {"UnknownEvent41", "UnknownEvent41"}, // 41
    {"UnknownEvent42", "UnknownEvent42"}, // 42
    {"MetaCall", "An asynchronous method invocation via QMetaObject::invokeMethod()"}, // 43
    {"UnknownEvent44", "UnknownEvent44"}, // 44
    {"UnknownEvent45", "UnknownEvent45"}, // 45
    {"UnknownEvent46", "UnknownEvent46"}, // 46
    {"UnknownEvent47", "UnknownEvent47"}, // 47
    {"UnknownEvent48", "UnknownEvent48"}, // 48
    {"UnknownEvent49", "UnknownEvent49"}, // 49
    {"SockAct", "Socket activated, used to implement QSocketNotifier"}, // 50
    {"ShortcutOverride", "Key press in child, for overriding shortcut key handling(QKeyEvent)"}, // 51
    {"DeferredDelete", "The object will be deleted after it has cleaned up (QDeferredDeleteEvent)"}, // 52
    {"UnknownEvent53", "UnknownEvent53"}, // 53
    {"UnknownEvent54", "UnknownEvent54"}, // 54
    {"UnknownEvent55", "UnknownEvent55"}, // 55
    {"UnknownEvent56", "UnknownEvent56"}, // 56
    {"UnknownEvent57", "UnknownEvent57"}, // 57
    {"UnknownEvent58", "UnknownEvent58"}, // 58
    {"UnknownEvent59", "UnknownEvent59"}, // 59
    {"DragEnter", "The cursor enters a widget during a drag and drop operation (QDragEnterEvent)"}, // 60
    {"DragMove", "A drag and drop operation is in progress (QDragMoveEvent)"}, // 61
    {"DragLeave", "The cursor leaves a widget during a drag and drop operation (QDragLeaveEvent)"}, // 62
    {"Drop", "A drag and drop operation is completed (QDropEvent)"}, // 63
    {"UnknownEvent64", "UnknownEvent64"}, // 64
    {"UnknownEvent65", "UnknownEvent65"}, // 65
    {"UnknownEvent66", "UnknownEvent66"}, // 66
    {"UnknownEvent67", "UnknownEvent67"}, // 67
    {"ChildAdded", "An object gets a child (QChildEvent)"}, // 68
    {"ChildPolished", "A widget child gets polished (QChildEvent)"}, // 69
    {"UnknownEvent70", "UnknownEvent70"}, // 70
    {"ChildRemoved", "An object loses a child (QChildEvent)"}, // 71
    {"UnknownEvent72", "UnknownEvent72"}, // 72
    {"UnknownEvent73", "UnknownEvent73"}, // 73
    {"PolishRequest", "The widget should be polished"}, // 74
    {"Polish", "The widget is polished"}, // 75
    {"LayoutRequest", "Widget layout needs to be redone"}, // 76
    {"UpdateRequest", "The widget should be repainted"}, // 77
    {"UpdateLater", "The widget should be queued to be repainted at a later time"}, // 78
    {"UnknownEvent79", "UnknownEvent79"}, // 79
    {"UnknownEvent80", "UnknownEvent80"}, // 80
    {"UnknownEvent81", "UnknownEvent81"}, // 81
    {"ContextMenu", "Context popup menu (QContextMenuEvent)"}, // 82
    {"InputMethod", "An input method is being used (QInputMethodEvent)"}, // 83
    {"UnknownEvent84", "UnknownEvent84"}, // 84
    {"UnknownEvent85", "UnknownEvent85"}, // 85
    {"UnknownEvent86", "UnknownEvent86"}, // 86
    {"TabletMove", "Wacom tablet move(QTabletEvent)"}, // 87
    {"LocaleChange", "The system locale has changed"}, // 88
    {"LanguageChange", "The application translation changed"}, // 89
    {"LayoutDirectionChange", "The direction of layouts changed"}, // 90
    {"UnknownEvent91", "UnknownEvent91"}, // 91
    {"TabletPress", "Wacom tablet press(QTabletEvent)"}, // 92
    {"TabletRelease", "Wacom tablet release(QTabletEvent)"}, // 93
    {"OkRequest", "Ok button in decoration pressed. Supported only for Windows CE"}, // 94
    {"UnknownEvent95", "UnknownEvent95"}, // 95
    {"IconDrag", "The main icon of a window has been dragged away(QIconDragEvent)"}, // 96
    {"FontChange", "Widget's font has changed"}, // 97
    {"EnabledChange", "Widget's enabled state has changed"}, // 98
    {"ActivationChange", "A widget's top-level window activation state has changed"}, // 99
    {"StyleChange", "Widget's style has been changed"}, // 100
    {"IconTextChange", "Widget's icon text has been changed"}, // 101
    {"ModifiedChange", "Widgets modification state has been changed"}, // 102
    {"WindowBlocked", "The window is blocked by a modal dialog"}, // 103
    {"WindowUnblocked", "The window is unblocked after a modal dialog exited"}, // 104
    {"WindowStateChange", "The window's state(minimized, maximized or full - screen) has changed(QWindowStateChangeEvent)"}, // 105
    {"ReadOnlyChange", "Widget's read - only state has changed(since Qt 5.4)"}, // 106
    {"UnknownEvent107", "UnknownEvent107"}, // 107
    {"UnknownEvent108", "UnknownEvent108"}, // 108
    {"MouseTrackingChange", "The mouse tracking state has changed"}, // 109
    {"ToolTip", "A tooltip was requested(QHelpEvent)"}, // 110
    {"WhatsThis", "The widget should reveal WhatsThis help(QHelpEvent)"}, // 111
    {"StatusTip", "A status tip is requested(QStatusTipEvent)"}, // 112
    {"ActionChanged", "An action has been changed (QActionEvent)"}, // 113
    {"ActionAdded", "A new action has been added (QActionEvent)"}, // 114
    {"ActionRemoved", "An action has been removed (QActionEvent)"}, // 115
    {"FileOpen", "File open request (QFileOpenEvent)"}, // 116
    {"Shortcut", "Key press in child for shortcut key handling(QShortcutEvent)"}, // 117
    {"WhatsThisClicked", "A link in a widget's WhatsThis help was clicked"}, // 118
    {"UnknownEvent119", "UnknownEvent119"}, // 119
    {"ToolBarChange", "The toolbar button is toggled on OS X"}, // 120
    {"ApplicationActivate", "This enum has been deprecated. Use ApplicationStateChange instead"}, // 121
    {"ApplicationDeactivate", "This enum has been deprecated. Use ApplicationStateChange instead"}, // 122
    {"QueryWhatsThis", "The widget should accept the event if it has WhatsThis help"}, // 123
    {"EnterWhatsThisMode", "Send to toplevel widgets when the application enters WhatsThis mode"}, // 124
    {"LeaveWhatsThisMode", "Send to toplevel widgets when the application leaves WhatsThis mode"}, // 125
    {"ZOrderChange", "The widget's z - order has changed. This event is never sent to top level windows"}, // 126
    {"HoverEnter", "The mouse cursor enters a hover widget(QHoverEvent)"}, // 127
    {"HoverLeave", "The mouse cursor leaves a hover widget(QHoverEvent)"}, // 128
    {"HoverMove", "The mouse cursor moves inside a hover widget(QHoverEvent)"}, // 129
    {"UnknownEvent130", "UnknownEvent130"}, // 130
    {"ParentAboutToChange", "The widget parent is about to change"}, // 131
    {"WinEventAct", "A Windows - specific activation event has occurred"}, // 132
    {"UnknownEvent133", "UnknownEvent133"}, // 133
    {"UnknownEvent134", "UnknownEvent134"}, // 134
    {"UnknownEvent135", "UnknownEvent135"}, // 135
    {"UnknownEvent136", "UnknownEvent136"}, // 136
    {"UnknownEvent137", "UnknownEvent137"}, // 137
    {"UnknownEvent138", "UnknownEvent138"}, // 138
    {"UnknownEvent139", "UnknownEvent139"}, // 139
    {"UnknownEvent140", "UnknownEvent140"}, // 140
    {"UnknownEvent141", "UnknownEvent141"}, // 141
    {"UnknownEvent142", "UnknownEvent142"}, // 142
    {"UnknownEvent143", "UnknownEvent143"}, // 143
    {"UnknownEvent144", "UnknownEvent144"}, // 144
    {"UnknownEvent145", "UnknownEvent145"}, // 145
    {"UnknownEvent146", "UnknownEvent146"}, // 146
    {"UnknownEvent147", "UnknownEvent147"}, // 147
    {"UnknownEvent148", "UnknownEvent148"}, // 148
    {"UnknownEvent149", "UnknownEvent149"}, // 149
    {"EnterEditFocus", "An editor widget gains focus for editing. QT_KEYPAD_NAVIGATION must be defined"}, // 150
    {"LeaveEditFocus", "An editor widget loses focus for editing. QT_KEYPAD_NAVIGATION must be defined"}, // 151
    {"UnknownEvent152", "UnknownEvent152"}, // 152
    {"UnknownEvent153", "UnknownEvent153"}, // 153
    {"UnknownEvent154", "UnknownEvent154"}, // 154
    {"GraphicsSceneMouseMove", "Move mouse in a graphics scene(QGraphicsSceneMouseEvent)"}, // 155
    {"GraphicsSceneMousePress", "Mouse press in a graphics scene(QGraphicsSceneMouseEvent)"}, // 156
    {"GraphicsSceneMouseRelease", "Mouse release in a graphics scene(QGraphicsSceneMouseEvent)"}, // 157
    {"GraphicsSceneMouseDoubleClick", "Mouse press again(double click) in a graphics scene(QGraphicsSceneMouseEvent)"}, // 158
    {"GraphicsSceneContextMenu", "Context popup menu over a graphics scene(QGraphicsSceneContextMenuEvent)"}, // 159
    {"GraphicsSceneHoverEnter", "The mouse cursor enters a hover item in a graphics scene(QGraphicsSceneHoverEvent)"}, // 160
    {"GraphicsSceneHoverMove", "The mouse cursor moves inside a hover item in a graphics scene(QGraphicsSceneHoverEvent)"}, // 161
    {"GraphicsSceneHoverLeave", "The mouse cursor leaves a hover item in a graphics scene(QGraphicsSceneHoverEvent)"}, // 162
    {"GraphicsSceneHelp", "The user requests help for a graphics scene(QHelpEvent)"}, // 163
    {"GraphicsSceneDragEnter", "The cursor enters a graphics scene during a drag and drop operation(QGraphicsSceneDragDropEvent)"}, // 164
    {"GraphicsSceneDragMove", "A drag and drop operation is in progress over a scene(QGraphicsSceneDragDropEvent)"}, // 165
    {"GraphicsSceneDragLeave", "The cursor leaves a graphics scene during a drag and drop operation(QGraphicsSceneDragDropEvent)"}, // 166
    {"GraphicsSceneDrop", "A drag and drop operation is completed over a scene(QGraphicsSceneDragDropEvent)"}, // 167
    {"GraphicsSceneWheel", "Mouse wheel rolled in a graphics scene(QGraphicsSceneWheelEvent)"}, // 168
    {"KeyboardLayoutChange", "The keyboard layout has changed"}, // 169
    {"DynamicPropertyChange", "A dynamic property was added, changed, or removed from the object"}, // 170
    {"TabletEnterProximity", "Wacom tablet enter proximity event(QTabletEvent), sent to QApplication"}, // 171
    {"TabletLeaveProximity", "Wacom tablet leave proximity event(QTabletEvent), sent to QApplication"}, // 172
    {"NonClientAreaMouseMove", "A mouse move occurred outside the client area"}, // 173
    {"NonClientAreaMouseButtonPress", "A mouse button press occurred outside the client area"}, // 174
    {"NonClientAreaMouseButtonRelease", "A mouse button release occurred outside the client area"}, // 175
    {"NonClientAreaMouseButtonDblClick", "A mouse double click occurred outside the client area"}, // 176
    {"MacSizeChange", "The user changed his widget sizes(OS X only)"}, // 177
    {"ContentsRectChange", "The margins of the widget's content rect changed"}, // 178
    {"UnknownEvent179", "UnknownEvent179"}, // 179
    {"UnknownEvent180", "UnknownEvent180"}, // 180
    {"GraphicsSceneResize", "Widget was resized(QGraphicsSceneResizeEvent)"}, // 181
    {"GraphicsSceneMove", "Widget was moved(QGraphicsSceneMoveEvent)"}, // 182
    {"CursorChange", "The widget's cursor has changed"}, // 183
    {"ToolTipChange", "The widget's tooltip has changed"}, // 184
    {"UnknownEvent185", "UnknownEvent185"}, // 185
    {"GrabMouse", "Item gains mouse grab(QGraphicsItem only)"}, // 186
    {"UngrabMouse", "Item loses mouse grab(QGraphicsItem only)"}, // 187
    {"GrabKeyboard", "Item gains keyboard grab(QGraphicsItem only)"}, // 188
    {"UngrabKeyboard", "Item loses keyboard grab(QGraphicsItem only)"}, // 189
    {"UnknownEvent190", "UnknownEvent190"}, // 190
    {"UnknownEvent191", "UnknownEvent191"}, // 191
    {"StateMachineSignal", "A signal delivered to a state machine(QStateMachine::SignalEvent)"}, // 192
    {"StateMachineWrapped", "The event is a wrapper for, i.e., contains, another event(QStateMachine::WrappedEvent)"}, // 193
    {"TouchBegin", "Beginning of a sequence of touch - screen or track - pad events(QTouchEvent)"}, // 194
    {"TouchUpdate", "Touch - screen event(QTouchEvent)"}, // 195
    {"TouchEnd", "End of touch - event sequence(QTouchEvent)"}, // 196
    {"NativeGesture", "The system has detected a gesture(QNativeGestureEvent)"}, // 197
    {"Gesture", "A gesture was triggered(QGestureEvent)"}, // 198
    {"RequestSoftwareInputPanel", "A widget wants to open a software input panel(SIP)"}, // 199
    {"CloseSoftwareInputPanel", "A widget wants to close the software input panel (SIP)"}, // 200
    {"UnknownEvent201", "UnknownEvent201"}, // 201
    {"GestureOverride", "A gesture override was triggered(QGestureEvent)"}, // 202
    {"WinIdChange", "The window system identifer for this native widget has changed"}, // 203
    {"ScrollPrepare", "The object needs to fill in its geometry information(QScrollPrepareEvent)"}, // 204
    {"Scroll", "The object needs to scroll to the supplied position(QScrollEvent)"}, // 205
    {"Expose", "Sent to a window when its on-screen contents are invalidated and need to be flushed from the backing store"}, // 206
    {"InputMethodQuery", "A input method query event (QInputMethodQueryEvent)"}, // 207
    {"OrientationChange", "The screens orientation has changes(QScreenOrientationChangeEvent)"}, // 208
    {"TouchCancel", "Cancellation of touch - event sequence(QTouchEvent)"}, // 209
    {"UnknownEvent210", "UnknownEvent210"}, // 210
    {"UnknownEvent211", "UnknownEvent211"}, // 211
    {"PlatformPanel", "A platform specific panel has been requested"}, // 212
    {"UnknownEvent213", "UnknownEvent213"}, // 213
    {"ApplicationStateChange", "The state of the application has changed"}, // 214
    {"UnknownEvent215", "UnknownEvent215"}, // 215
    {"UnknownEvent216", "UnknownEvent216"}, // 216
    {"PlatformSurface", "A native platform surface has been created or is about to be destroyed(QPlatformSurfaceEvent)"}, // 217
};

char const *event_type_2_name_latin1lit(int e)
{
    if(e < 0) {
        PROGRAMMERERROR("event_type_2_name_latin1lit: given invalid type %d", e);
    }

    if(e >= 1000 && e <= 65535) {
        return "UserEvent";
    }

    if(e >= MAXN) {
        return "UnknownLargeEvent";
    }

    char const *name = events_type_infos[e].name;

    if(name == NULL) {
        return "UnknownEvent";
    }

    return name;
}
char const *event_type_2_desc(int e)
{
    if(e < 0) {
        PROGRAMMERERROR("event_type_2_desc: given invalid type %d", e);
    }

    if(e >= 1000 && e <= 65535) {
        return "UserEvent";
    }

    if(e >= MAXN) {
        return "UnknownLargeEvent";
    }

    char const *desc = events_type_infos[e].desc;

    if(desc == NULL) {
        return "UnknownEvent";
    }

    return desc;
}
