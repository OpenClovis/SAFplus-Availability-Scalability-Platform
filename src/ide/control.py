# Scintilla-based text editor implemented in Python + wxPython.
# Source https://github.com/fogleman/TextEditor

import wx
import wx.stc as stc
import os
import util
from settings import settings

class EditorEvent(wx.PyEvent):
    def __init__(self, type, object=None):
        super(EditorEvent, self).__init__()
        self.SetEventType(type.typeId)
        self.SetEventObject(object)
        
EVT_EDITOR_STATUS_CHANGED = wx.PyEventBinder(wx.NewEventType())

class EditorControl(stc.StyledTextCtrl):
    creation_counter = 0
    LINE_MARGIN = 0
    BOOKMARK_MARGIN = 1
    FOLDING_MARGIN = 2
    def __init__(self, parent, style, file_path):
        stc.StyledTextCtrl.__init__(self, parent)
        EditorControl.creation_counter += 1
        self._id = EditorControl.creation_counter
        self._line_count = -1
        self._edited = False
        self._markers = {}
        self._indic_dirty = [False]*3
        self._recording = False
        self._macro = []
        self.language = None
        self.file_path = file_path
        self.open_file(file_path)
        self.mark_stat()
        self.apply_settings()
        self.SetEOLMode(stc.STC_EOL_LF)
        self.SetModEventMask(stc.STC_MOD_INSERTTEXT | stc.STC_MOD_DELETETEXT | stc.STC_PERFORMED_USER | stc.STC_PERFORMED_UNDO | stc.STC_PERFORMED_REDO)
        self.Bind(stc.EVT_STC_CHANGE, self.on_change)
        self.Bind(stc.EVT_STC_UPDATEUI, self.on_updateui)
        self.Bind(stc.EVT_STC_CHARADDED, self.on_charadded)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.on_marginclick)
        self.Bind(stc.EVT_STC_MACRORECORD, self.on_macrorecord)
        self.Bind(wx.EVT_RIGHT_UP, self.on_right_up)
    def get_name(self):
        if self.file_path:
            pre, name = os.path.split(self.file_path)
            return name
        else:
            return '(Untitled-%d)' % self._id
    def get_edited(self):
        return self._edited
    def set_edited(self, edited):
        if self._edited != edited:
            self._edited = edited
            wx.PostEvent(self, EditorEvent(EVT_EDITOR_STATUS_CHANGED, self))
    edited = property(get_edited, set_edited)
    def get_frame(self):
        # notebook = self.GetParent()
        # frame = notebook.GetParent()
        # return frame
        return self.GetParent()
    def confirm_close(self, can_veto=True):
        if self.edited and settings.CONFIRM_CLOSE_WITH_EDITS:
            frame = self.get_frame()
            name = self.file_path or self.get_name()
            style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION
            if can_veto:
                style |= wx.CANCEL
            self.SetFocus()
            dialog = wx.MessageDialog(frame, 'Save changes to file "%s"?' % name, 'Save Changes?', style)
            result = dialog.ShowModal()
            if result == wx.ID_YES:
                frame.onSave()
            elif result == wx.ID_CANCEL:
                return False
        return True
    def apply_settings(self):
        self.IndicatorSetStyle(1, stc.STC_INDIC_ROUNDBOX)
        self.IndicatorSetForeground(1, wx.RED)
        self.IndicatorSetStyle(2, stc.STC_INDIC_ROUNDBOX)
        self.IndicatorSetForeground(2, wx.BLUE)
        self.SetCaretForeground(settings.CARET_FOREGROUND)
        self.SetCaretLineVisible(settings.CARET_LINE_VISIBLE)
        self.SetCaretLineBack(settings.CARET_LINE_BACKGROUND)
        self.SetCaretPeriod(settings.CARET_PERIOD)

        self.SetCaretWidth(settings.CARET_WIDTH)
        self.SetWrapMode(stc.STC_WRAP_WORD if settings.WORD_WRAP else stc.STC_WRAP_NONE)
        self.SetSelBackground(bool(settings.SELECTION_BACKGROUND), settings.SELECTION_BACKGROUND)
        self.SetSelForeground(bool(settings.SELECTION_FOREGROUND), settings.SELECTION_FOREGROUND)
        self.SetUseHorizontalScrollBar(settings.USE_HORIZONTAL_SCROLL_BAR)
        self.SetBackSpaceUnIndents(settings.BACKSPACE_UNINDENTS)
        
        # self.SetEdgeMode(settings.EDGE_MODE)
        self.SetEndAtLastLine(settings.END_AT_LAST_LINE)
        self.SetIndentationGuides(settings.INDENTATION_GUIDES)

        self.SetTabIndents(settings.TAB_INDENTS)
        self.SetTabWidth(settings.TAB_WIDTH)
        self.SetUseTabs(settings.USE_TABS)

        # self.SetEdgeColumn(settings.EDGE_COLUMN)
        # self.SetViewWhiteSpace(settings.VIEW_WHITESPACE)
        self.SetMargins(settings.MARGIN_LEFT, settings.MARGIN_RIGHT)
        
        self.apply_bookmark_settings()
        self.apply_folding_settings()
        self.show_line_numbers()
        self.detect_language()
    def apply_bookmark_settings(self):
        if True:
            self.SetMarginType(self.BOOKMARK_MARGIN, stc.STC_MARGIN_SYMBOL)
            self.SetMarginSensitive(self.BOOKMARK_MARGIN, True)
            self.SetMarginWidth(self.BOOKMARK_MARGIN, settings.BOOKMARK_MARGIN_SIZE)
        else:
            self.SetMarginSensitive(self.BOOKMARK_MARGIN, False)
            self.SetMarginWidth(self.BOOKMARK_MARGIN, 0)
    def apply_folding_settings(self):
        if True:
            self.SetProperty("fold", "1")
            self.SetMarginType(self.FOLDING_MARGIN, stc.STC_MARGIN_SYMBOL)
            # self.SetMarginMask(self.FOLDING_MARGIN, stc.STC_MASK_FOLDERS)
            self.SetMarginSensitive(self.FOLDING_MARGIN, True)
            self.SetMarginWidth(self.FOLDING_MARGIN, settings.FOLDING_MARGIN_SIZE)
            self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND, stc.STC_MARK_BOXPLUSCONNECTED, "white", "#666666")
            self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_BOXMINUSCONNECTED, "white", "#666666")
            self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_TCORNER, "white", "#666666")
            self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL, stc.STC_MARK_LCORNER, "white", "#666666")
            self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB, stc.STC_MARK_VLINE, "white", "#666666")
            self.MarkerDefine(stc.STC_MARKNUM_FOLDER, stc.STC_MARK_BOXPLUS, "white", "#666666")
            self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN, stc.STC_MARK_BOXMINUS, "white", "#666666")
        else:
            self.SetProperty("fold", "0")
            self.SetMarginSensitive(self.FOLDING_MARGIN, False)
            self.SetMarginWidth(self.FOLDING_MARGIN, 0)
    def get_stat(self):
        path = self.file_path
        if path:
            stat = os.stat(path)
            return (stat.st_size, stat.st_mtime)
        else:
            return None
    def mark_stat(self):
        self._stat = self.get_stat()
    def check_stat(self):
        stat = self.get_stat()
        return stat == self._stat
    def open_file(self, path):
        file = None
        try:
            file = open(path, 'r')
            text = file.read()
            self.SetText(text)
            self.EmptyUndoBuffer()
            self.edited = False
            self.file_path = path
        except IOError:
            self.SetText('')
        finally:
            if file:
                file.close()
            self.mark_stat()
            self.detect_language()
            self.update_line_numbers()
    def save_file(self, path=None, force=False):
        path = path or self.file_path
        if not path:
            return False
        if not self.edited and not force:
            return True
        file = None
        try:
            file = open(path, 'w')
            text = self.GetText()
            file.write(text)
            self.edited = False
            self.file_path = path
            return True
        finally:
            if file:
                file.close()
            self.mark_stat()
            self.detect_language()
        return False
    def reload_file(self):
        if self.file_path:
            self.open_file(self.file_path)
    def detect_language(self):
        path = self.file_path
        manager = self.get_frame().style_manager
        if path:
            pre, ext = os.path.splitext(path)
            if not ext:
                ext = str(pre).split('/')[-1]
            language = manager.get_language(ext)
        else:
            language = None
        self.apply_language(manager, language)
    def apply_language(self, manager, language):
        self.language = language
        self.ClearDocumentStyle()
        self.SetKeyWords(0, '')
        self.SetLexer(stc.STC_LEX_NULL)
        self.StyleResetDefault()
        self.apply_style(manager.base_style)
        self.StyleClearAll()
        self.apply_styles(manager.app_styles)
        if language:
            self.SetLexer(language.lexer)
            self.SetKeyWords(0, ' '.join(language.keywords.split()))
            for n in range(2, 8):
                attr = 'keywords%d' % n
                if not hasattr(language, attr):
                    continue
                keywords = getattr(language, attr)
                self.SetKeyWords(n-1, ' '.join(keywords.split()))
            self.apply_styles(language.styles)
        self.Colourise(0, self.GetLength())
    def apply_styles(self, styles):
        for style in styles:
            self.apply_style(style)
    def apply_style(self, style):
        s = style
        id = s.number
        self.StyleSetFontAttr(id, s.size, s.font, s.bold, s.italic, s.underline)
        self.StyleSetBackground(id, s.create_background())
        self.StyleSetForeground(id, s.create_foreground())
    def match_brace(self):
        invalid = stc.STC_INVALID_POSITION
        if settings.MATCH_BRACES:
            for i in (1, 0):
                a = self.GetCurrentPos() - i
                b = self.BraceMatch(a)
                if b != invalid:
                    self.BraceHighlight(a, b)
                    return
        self.BraceHighlight(invalid, invalid)
    def show_line_numbers(self):
        self.SetMarginType(self.LINE_MARGIN, stc.STC_MARGIN_NUMBER)
        # self.SetMarginWidth(self.LINE_MARGIN, 0)
        self.update_line_numbers()
    def update_line_numbers(self):
        if settings.LINE_NUMBERS:
            lines = self.GetLineCount()
            if lines != self._line_count:
                self._line_count = lines
                text = '%d ' % lines
                n = len(text)
                if n < 4: text += ' ' * (4-n)
                width = self.TextWidth(stc.STC_STYLE_LINENUMBER, text)
                self.SetMarginWidth(self.LINE_MARGIN, width)
    def lower(self):
        text = self.GetSelectedText()
        self.ReplaceSelection(text.lower())
    def upper(self):
        text = self.GetSelectedText()
        self.ReplaceSelection(text.upper())
    def sort(self):
        text = self.GetSelectedText()
        lines = text.split('\n')
        lines.sort()
        self.ReplaceSelection('\n'.join(lines))
    def indent(self):
        self.CmdKeyExecute(stc.STC_CMD_TAB)
    def unindent(self):
        self.CmdKeyExecute(stc.STC_CMD_BACKTAB)
    def toggle_comment(self):
        if not self.language:
            return
        comment = self.language.line_comment
        if not comment:
            return
        self.BeginUndoAction()
        a, b = self.GetSelection()
        start, end = self.LineFromPosition(a), self.LineFromPosition(b)
        if self.PositionFromLine(end) == b:
            end -= 1
        for line in range(start, end+1):
            text = self.GetLine(line)
            if text.startswith(comment):
                text = text[len(comment):]
            else:
                text = comment + text
            a, b = self.PositionFromLine(line), self.PositionFromLine(line+1)
            self.SetSelection(a, b)
            self.ReplaceSelection(text)
        a, b = self.PositionFromLine(start), self.PositionFromLine(end+1)
        self.SetSelection(a, b)
        self.EndUndoAction()
    def word_wrap(self):
        mode = self.GetWrapMode()
        if mode == stc.STC_WRAP_WORD:
            self.SetWrapMode(stc.STC_WRAP_NONE)
            return False
        else:
            self.SetWrapMode(stc.STC_WRAP_WORD)
            return True
    def find(self, text=None, previous=False, wrap=True, flags=0, use_selection=True):
        if use_selection and settings.USE_SELECTION_FOR_F3:
            text = self.GetSelectedText() or text
        if text:
            pos = self.GetSelectionStart() if previous else self.GetSelectionEnd()
            wrap_pos = self.GetLength() if previous else 0
            for index in (pos, wrap_pos):
                self.SetSelection(index, index)
                self.SearchAnchor()
                func = self.SearchPrev if previous else self.SearchNext
                if self._recording:
                    code = -2368 if previous else -2367
                    command = (code, flags, text)
                    self._macro.append(command)
                if func(flags, text) >= 0:
                    break
                if not wrap:
                    break
            else:
                self.SetSelection(pos, pos)
            self.EnsureCaretVisible()
    def replace_all(self, text, replacement, flags=0, in_selection=False):
        if not text:
            return
        length = len(text)
        rlength = len(replacement)
        if in_selection:
            start, end = self.GetSelection()
        else:
            start, end = 0, self.GetLength()
        index = start
        self.BeginUndoAction()
        while True:
            index = self.FindText(index, self.GetLength(), text, flags)
            if index < 0: break
            if index + length > end: break
            self.SetSelection(index, index+length)
            self.ReplaceSelection(replacement)
            index += rlength
        self.EndUndoAction()
        
    def get_indicator_mask(self, indicator):
        if indicator == 0: return stc.STC_INDIC0_MASK
        if indicator == 1: return stc.STC_INDIC1_MASK
        if indicator == 2: return stc.STC_INDIC2_MASK
        return 0
    def clear_indicator(self, indicator):
        if self._indic_dirty[indicator]:
            self._indic_dirty[indicator] = False
            mask = self.get_indicator_mask(indicator)
            self.StartStyling(0, mask)
            self.SetStyling(self.GetLength(), 0)
    def highlight_range(self, indicator, start, length):
        mask = self.get_indicator_mask(indicator)
        self.StartStyling(start, mask)
        self.SetStyling(length, mask)
    def highlight_all(self, indicator, text, flags):
        if not text:
            return
        index = 0
        length = len(text)
        start = self.GetSelectionStart()
        count = 0
        while True:
            index = self.FindText(index, self.GetLength(), text, flags)
            if index < 0: break
            if index != start:
                self.highlight_range(indicator, index, length)
                count += 1
            index += 1
        if count > 0:
            self._indic_dirty[indicator] = True
        return count
    def highlight_selection(self):
        indicator = 1
        self.clear_indicator(indicator)
        if not settings.HIGHLIGHT_SELECTION:
            return
        text = self.GetSelectedText()
        if not text or '\n' in text:
            return
        if text in self._markers:
            return
        flags = stc.STC_FIND_WHOLEWORD | stc.STC_FIND_MATCHCASE
        self.highlight_all(indicator, text, flags)
    def highlight_markers(self):
        indicator = 2
        self.clear_indicator(indicator)
        for text, flags in self._markers.iteritems():
            self.highlight_all(indicator, text, flags)
    def mark_text(self, text=None, flags=None):
        text = text or self.GetSelectedText()
        if flags is None:
            flags = stc.STC_FIND_WHOLEWORD | stc.STC_FIND_MATCHCASE
        if text:
            self._markers[text] = flags
            self.highlight_markers()
    def unmark_text(self, text=None):
        text = text or self.GetSelectedText()
        if text in self._markers:
            del self._markers[text]
            self.highlight_markers()
    def unmark_all(self):
        self._markers = {}
        self.highlight_markers()
        
    def on_right_up(self, event):
        frame = self.get_frame()
        menu = frame.create_context_menu(self)
        self.PopupMenu(menu, event.GetPosition())
    def on_change(self, event):
        self.edited = True
    def on_charadded(self, event):
        code = event.GetKey()
        if self._recording:
            command = (2170, code, 0)
            self._macro.append(command)
        if code == ord('\n'): # auto indent
            line = self.GetCurrentLine() - 1
            if line >= 0:
                text = self.GetLine(line)
                index = -1
                for i, ch in enumerate(text):
                    if ch in (' ', '\t'):
                        index = i
                    else:
                        break
                if index >= 0:
                    self.ReplaceSelection(text[:index+1])
    def on_marginclick(self, event):
        margin = event.GetMargin()
        line = self.LineFromPosition(event.GetPosition())
        if margin == self.BOOKMARK_MARGIN:
            marker = 1
            symbol = settings.BOOKMARK_SYMBOL
            self.MarkerDefine(marker, symbol)
            self.MarkerSetBackground(marker, 'light blue')
            if self.MarkerGet(line) & (1 << marker):
                self.MarkerDelete(line, marker)
            else:
                self.MarkerAdd(line, marker)
        if margin == self.FOLDING_MARGIN:
            self.ToggleFold(line)
    def on_updateui(self, event):
        self.match_brace()
        self.update_line_numbers()
        self.highlight_selection()
        self.highlight_markers()
        
    def toggle_macro(self):
        if self._recording:
            self.stop_macro()
        else:
            self.start_macro()
    def start_macro(self):
        self._recording = True
        self._macro = []
        self.StartRecord()
    def stop_macro(self):
        self._recording = False
        self.StopRecord()
    def play_macro_to_end(self):
        if not self._macro:
            return
        prev_line = -1
        count = 0
        while True:
            self.play_macro()
            new_line = self.GetCurrentLine()
            if new_line == prev_line:
                count += 1
                if count > 10:
                    break
            else:
                count = 0
            prev_line = new_line
            pos = self.GetCurrentPos()
            end = self.GetLength()
            text = self.GetTextRange(pos, end).strip()
            if not text:
                break
    def play_macro(self):
        codes = [
            2011,2013,2176,2177,2178,2179,2180,2300,2301,2302,2303,2304,2305,
            2306,2307,2308,2309,2310,2311,2312,2313,2314,2315,2316,2317,2318,
            2319,2320,2321,2322,2323,2324,2325,2326,2327,2328,2329,2330,2331,
            2332,2333,2334,2335,2336,2337,2338,2339,2404,2340,2341,2342,2343,
            2344,2345,2346,2347,2348,2349,2450,2451,2452,2453,2454,2455,2390,
            2391,2392,2393,2395,2396,2413,2414,2415,2416,2426,2427,2428,2429,
            2430,2431,2432,2433,2434,2435,2436,2437,2438,2439,2440,2441,2442,
        ]
        if self._macro:
            self.BeginUndoAction()
            for command in self._macro:
                message, wparam, lparam = command
                if message == 2170: #SCI_REPLACESEL
                    self.ReplaceSelection(chr(wparam))
                elif message == 2366: #SCI_SEARCHANCHOR
                    self.SearchAnchor()
                elif message == -2367: #SCI_SEARCHNEXT
                    self.find(flags=wparam, text=lparam)
                elif message == -2368: #SCI_SEARCHPREV
                    self.find(flags=wparam, text=lparam, previous=True)
                elif message in codes:
                    self.CmdKeyExecute(message)
                else:
                    pass #print 'Unhandled Macro Code:', message
            self.EndUndoAction()
    def on_macrorecord(self, event):
        command = (event.GetMessage(), event.GetWParam(), event.GetLParam())
        self._macro.append(command)
        