import wx
import wx.stc as stc
import re

def create_color(red, green, blue):
    return wx.Colour(red, green, blue)

def create_font(name, size, bold=False, italic=False, underline=False):
    weight = wx.FONTWEIGHT_BOLD if bold else wx.FONTWEIGHT_NORMAL
    style = wx.FONTSTYLE_ITALIC if italic else wx.FONTSTYLE_NORMAL
    font = wx.Font(size, wx.FONTFAMILY_DEFAULT, style, weight, underline, name)
    return font

class Style(object):
    def __init__(self, parent=None, number=None, name=None, preview=None, 
        font=None, size=None, bold=None, italic=None, underline=None, 
        foreground=None, background=None):
        self._parent = parent
        self._number = number
        self._name = name
        self._preview = preview or name
        self._font = font
        self._size = size
        self._bold = bold
        self._italic = italic
        self._underline = underline
        self._foreground = foreground
        self._background = background
    def __cmp__(self, other):
        return cmp(self.preview, other.preview)
    def __setattr__(self, name, value):
        if name.startswith('_'):
            super(Style, self).__setattr__(name, value)
        else:
            setattr(self, '_%s' % name, value)
    def __getattr__(self, name):
        if name.startswith('_'):
            return super(Style, self).__getattr__(name)
        value = getattr(self, '_%s' % name)
        if value is None and self._parent:
            return getattr(self._parent, name)
        return value
    def clear(self):
        if self._parent is None:
            return
        self.font = None
        self.size = None
        self.bold = None
        self.italic = None
        self.underline = None
        self.foreground = None
        self.background = None
    def create_font(self):
        return create_font(self.font, self.size, self.bold, self.italic, self.underline)
    def create_foreground(self):
        return create_color(*self.foreground)
    def create_background(self):
        return create_color(*self.background)

class Language(object):
    def __init__(self, name, extensions=[], lexer=stc.STC_LEX_NULL, 
        base_style=None, styles=[], keywords='', keywords2='', keywords3='',
        line_comment='', block_comment=('', '')):
        self.name = name
        self.extensions = extensions
        self.lexer = lexer
        self.base_style = base_style
        self.styles = styles
        self.keywords = keywords
        self.keywords2 = keywords2
        self.keywords3 = keywords3
        self.line_comment = line_comment
        self.block_comment = block_comment
    def copy_from(self, other):
        self.extensions = other.extensions
        self.lexer = other.lexer
        self.keywords = other.keywords
        self.keywords2 = other.keywords2
        self.keywords3 = other.keywords3
        self.line_comment = other.line_comment
        self.block_comment = other.block_comment
    def __cmp__(self, other):
        return cmp(self.name, other.name)

class UnsupportedLanguage(Language):
    # Languages that don't have build-in wxpython lexer or custom lexer is needed
    # Based on https://wiki.wxwidgets.org/Adding_a_custom_lexer_with_syntax_highlighting_and_folding_to_a_WxStyledTextCtrl
    def __init__(self, name, extensions=[], base_style=None, styles=[], keywords='', keywords2='', keywords3='', line_comment='', block_comment=('', '')):
        super().__init__(name, extensions, stc.STC_LEX_CONTAINER, base_style, styles, keywords, keywords2, keywords3, line_comment, block_comment)

    def style_routine(self, evt):
        # Should be bound to stc.EVT_STC_STYLENEEDED of EditorControl
        control = evt.GetEventObject()  # EditorControl itself

        if control.edited == False:
            return

        line_start = control.LineFromPosition(control.GetEndStyled())   # line of the last styled text
        line_end = control.GetCurrentLine() # line containing the caret

        if line_end < line_start:   # swap
            line_start = line_start + line_end
            line_end = line_start - line_end
            line_start = line_start - line_end

        if line_start > 0:
            line_start -= 1

        if line_end < control.GetLineCount() - 1:
            line_end += 1

        start_pos = control.PositionFromLine(line_start)
        end_pos = control.GetLineEndPosition(line_end)
        target_text = control.GetTextRange(start_pos, end_pos)

        tokens = self.tokenize(start_pos, end_pos, target_text)
        self.apply_style(tokens, control)

    def tokenize(self, start_pos, end_pos, text):
        # Virtual method for custom lexer; parsing keywords, comments, targets, ...
        # ouput: list of tuples (start pos of text portion, end pos, style id for that token)
        pass

    def apply_style(self, tokens, control):
        # Apply color to tokens
        for (start_pos, end_pos, style_id) in tokens:
            length = end_pos - start_pos
            control.StartStyling(start_pos)
            control.SetStyling(length, style_id)

class Makefile(UnsupportedLanguage):
    MK_TARGET = 10
    MK_KEYWORD = 11
    MK_COMMENT = 12
    MK_FUNCTION = 13
    MK_VAR_REF = 14
    MK_SPECIAL_VAR = 15
    MK_DEFAULT = 16

    keywords = ["ifeq", "ifneq", "ifdef", "ifndef", "endif", "else", "include", "define"]
    build_in_funcs = ["subst", "patsubst", "strip", "join", "sort", "filter",
                      "filter-out", "findstring", "wildcard", "basename", "error",
                      "dirname", "suffix", "shell", "origin", "value", "addsuffix"]
    patterns = [
        (MK_VAR_REF, r"[$][(](\S*?)[)]"),
        (MK_TARGET, r"(.*?)[:][^\=]"),
        (MK_KEYWORD, "(" + "|".join(keywords) + ")"),
        (MK_COMMENT, r"([#].*)"),
        (MK_FUNCTION, "(" + "|".join(build_in_funcs) + ")"),
        (MK_SPECIAL_VAR, r"(\$\@|\$\<|\$\^|\$\?|\$\*)")
    ]

    def __init__(self, base_style=None):
        styles = [
            Style(base_style, Makefile.MK_DEFAULT, "Default"),
            Style(base_style, Makefile.MK_TARGET, "Target"),
            Style(base_style, Makefile.MK_KEYWORD, "Keyword"),
            Style(base_style, Makefile.MK_COMMENT, "Comment"),
            Style(base_style, Makefile.MK_FUNCTION, "Built-in Function"),
            Style(base_style, Makefile.MK_VAR_REF, "Referenced Variable"),
            Style(base_style, Makefile.MK_SPECIAL_VAR, "Special Variable")
        ]
        super().__init__(name = "Makefile",
                         extensions = ["makefile"],
                         base_style = base_style,
                         styles = styles,
                         keywords = '',
                         keywords2 = '',
                         keywords3 = '',
                         line_comment = '#',
                         block_comment = ('', ''))

    def tokenize(self, start_pos, end_pos, text):
        tokens = [(start_pos, end_pos, Makefile.MK_DEFAULT)]
        group = 1
        for (id, pattern) in Makefile.patterns:
            for match in re.finditer(pattern, text):
                tokens.append((start_pos + match.start(group), start_pos + match.end(group), id))
        return tokens

class Yang(UnsupportedLanguage):
    YG_KEYWORD = 10
    YG_DEFAULT = 11
    YG_DATA_TYPE = 12
    YG_COMMENT = 13
    YG_OPERATOR = 14
    YG_STRING = 15
    YG_NUMBER = 16

    PRIMARY_GRP = 1
    CMT_GRP_TRUTHY = 2
    CMT_GRP_FALSY = 3

    keywords = ["module", "submodule", "namespace", "prefix", "import", "include""typedef",
                "grouping", "container", "list", "leaf", "leaf-list", "choice", "case",
                "notification", "rpc", "action", "input", "output", "status", "description",
                "default", "mandatory", "key", "augment", "when", "organization", "contact",
                "revision", "typedef", "type", "enum"]

    build_in_types = ["binary", "bits", "boolean", "decimal64", "empty", "enumeration", "identityref",
                      "instance-identifier", "int8", "int16", "int32", "int64", "leafref", "string",
                      "uint8", "uint16", "uint32", "uint64", "union"]

    patterns = [
        (YG_KEYWORD, "("+ "|".join(keywords) +")", PRIMARY_GRP),
        (YG_STRING, r"(\".*?\")", PRIMARY_GRP),
        (YG_NUMBER, r"(\d*)", PRIMARY_GRP),
        (YG_DATA_TYPE, "("+ "|".join(build_in_types) +")", PRIMARY_GRP),
        (YG_OPERATOR, r"(:|=)", PRIMARY_GRP),
        (YG_COMMENT, r"(\/[*][\S\s]*?[*]\/)", PRIMARY_GRP),
        (YG_COMMENT, r"(\n[^\"\n]*)?(?(1)(//[^\n]*)|\"[^\n]*\"[^\n]*(//[^\n]*))", CMT_GRP_TRUTHY)  # conditional pattern used for single-line comment
    ]

    def __init__(self, base_style=None):
        styles = [
            Style(base_style, Yang.YG_DEFAULT, "Default"),
            Style(base_style, Yang.YG_KEYWORD, "Keyword"),
            Style(base_style, Yang.YG_DATA_TYPE, "Build-in Type"),
            Style(base_style, Yang.YG_COMMENT, "Comment"),
            Style(base_style, Yang.YG_OPERATOR, "Operator"),
            Style(base_style, Yang.YG_STRING, "String"),
            Style(base_style, Yang.YG_NUMBER, "Number")
        ]
        super().__init__(name = "Yang",
                         extensions = ["yang"],
                         base_style = base_style,
                         styles = styles,
                         keywords = '',
                         keywords2 = '',
                         keywords3 = '',
                         line_comment = '//',
                         block_comment = ('/*', '*/'))

    def tokenize(self, start_pos, end_pos, text):
        text = "\n" + text  # Make single-line comments easier to handle
        offset = 1  # because of the newline char above

        tokens = [(start_pos, end_pos, Yang.YG_DEFAULT)]
        for (style_id, pattern, group) in Yang.patterns:
            for match in re.finditer(pattern, text):
                gr_tmp = group
                if style_id == Yang.YG_COMMENT and group == Yang.CMT_GRP_TRUTHY:
                    if not match.group(Yang.CMT_GRP_TRUTHY):
                        gr_tmp = Yang.CMT_GRP_FALSY # result is stored in falsy group

                tokens.append((start_pos + match.start(gr_tmp) - offset,
                               start_pos + match.end(gr_tmp) - offset,
                               style_id))
        return tokens
