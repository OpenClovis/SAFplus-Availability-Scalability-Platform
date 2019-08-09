import wx
import wx.aui as aui
import wx.stc as stc

APP_NAME = 'Text Editor'

AUTO_INDENT = True
BACKSPACE_UNINDENTS = True
BOOKMARKS = True
BOOKMARK_MARGIN_SIZE = 20
BOOKMARK_SYMBOL = stc.STC_MARK_ARROW
CARET_FOREGROUND = wx.BLACK
CARET_LINE_VISIBLE = True
CARET_LINE_BACKGROUND = '#eeeeee'
CARET_PERIOD = 500
CARET_WIDTH = 1
EDGE_COLUMN = 80
EDGE_MODE = stc.STC_EDGE_LINE
END_AT_LAST_LINE = True
FOLDING = True
FOLDING_MARGIN_SIZE = 12
HIGHLIGHT_SELECTION = True
HIGHLIGHT_SYNTAX = True
INDENTATION_GUIDES = True
LINE_NUMBERS = True
MARGIN_LEFT = 2
MARGIN_RIGHT = 2
MATCH_BRACES = True
SELECTION_BACKGROUND = 'light grey'
SELECTION_FOREGROUND = 'black'
TAB_INDENTS = True
TAB_WIDTH = 4
USE_HORIZONTAL_SCROLL_BAR = True
USE_SELECTION_FOR_F3 = True
USE_TABS = False
VIEW_WHITESPACE = True
WORD_WRAP = False

AUI_NB_TAB_FIXED_WIDTH = False
AUI_NB_SCROLL_BUTTONS = True
AUI_NB_WINDOWLIST_BUTTON = True
AUI_NB_CLOSE = aui.AUI_NB_CLOSE_BUTTON
#AUI_NB_CLOSE_BUTTON
#AUI_NB_CLOSE_ON_ACTIVE_TAB
#AUI_NB_CLOSE_ON_ALL_TABS
AUI_NB_POSITION = aui.AUI_NB_TOP
#AUI_NB_TOP
#AUI_NB_BOTTOM

MAIN_WINDOW_MAXIMIZED = False
MAIN_WINDOW_PERSISTED = True
MAIN_WINDOW_POSITION = None
MAIN_WINDOW_SIZE = None

ACTIVE_TAB = -1
CLOSE_TAB_ON_DOUBLE_CLICK = True
CREATE_TAB_ON_DOUBLE_CLICK = True
CONFIRM_CLOSE_WITH_EDITS = True
FULL_PATH_IN_TITLE = True
OPEN_FILES = []
SHOW_RECENT_FILES = True
RECENT_FILES = []
RECENT_FILES_DISPLAY = 10
RECENT_FILES_SIZE = 25
REMEMBER_OPEN_FILES = True
SET_DIRECTORY_FOR_OPEN = True
USE_PSYCO = True
PYBROWSER_INFO = None

FIND_TEXT = ''
FIND_HISTORY = []
FIND_WHOLE_WORD = False
FIND_MATCH_CASE = False
FIND_NORMAL = True
FIND_EXTENDED = False
FIND_REGEX = False
FIND_CLOSE_DIALOG = False
FIND_DOWN = True
FIND_WRAP = True
REPLACE_TEXT = ''
REPLACE_HISTORY = []

CPP_KEYWORDS = \
'''
asm auto bool break case catch char class const const_cast
continue default delete do double dynamic_cast else enum explicit export
extern false float for friend goto if inline int long mutable namespace new
operator private protected public register reinterpret_cast return short signed
sizeof static static_cast struct switch template this throw true try typedef
typeid typename union unsigned using virtual void volatile wchar_t while
'''
