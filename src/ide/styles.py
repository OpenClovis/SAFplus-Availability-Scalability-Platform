# Scintilla-based text editor implemented in Python + wxPython.
# Source https://github.com/fogleman/TextEditor

import wx
import wx.stc as stc
import copy
import pickle
import util

STYLE_PATH = 'styles.dat'
DEFAULT_STYLE_PATH = 'default-styles.dat'

def create_color(red, green, blue):
    return wx.Colour(red, green, blue)
    
def create_font(name, size, bold=False, italic=False, underline=False):
    weight = wx.FONTWEIGHT_BOLD if bold else wx.FONTWEIGHT_NORMAL
    style = wx.FONTSTYLE_ITALIC if italic else wx.FONTSTYLE_NORMAL
    font = wx.Font(size, wx.FONTFAMILY_DEFAULT, style, weight, underline, name)
    return font
    
class StyleManager(object):
    def __init__(self):
        self.load()
        self.language_defaults = create_languages(self.base_style)
        self.cleanup()
    def load(self):
        for path in (STYLE_PATH, DEFAULT_STYLE_PATH):
            try:
                file = open(path, 'rb')
                pickler = pickle.Unpickler(file)
                self.base_style = pickler.load()
                self.app_styles = pickler.load()
                self.languages = pickler.load()
                file.close()
                return
            except:
                pass
        self.base_style = create_base_style()
        self.app_styles = create_app_styles(self.base_style)
        self.languages = create_languages(self.base_style)
    def save(self):
        try:
            file = open(STYLE_PATH, 'wb')
            pickler = pickle.Pickler(file, -1)
            pickler.dump(self.base_style)
            pickler.dump(self.app_styles)
            pickler.dump(self.languages)
            file.close()
        except:
            pass
    def cleanup(self):
        defaults = self.language_defaults
        languages = self.languages
        n1 = set(lang.name for lang in defaults)
        n2 = set(lang.name for lang in languages)
        new_names = n1 - n2
        old_names = n2 - n1
        to_add = [copy.deepcopy(lang) for lang in defaults if lang.name in new_names]
        to_remove = [lang for lang in languages if lang.name in old_names]
        for lang in to_remove:
            languages.remove(lang)
        languages.extend(to_add)
        
        for lang in languages:
            for default in defaults:
                if default.name == lang.name:
                    break
            else:
                continue
            lang.copy_from(default)
    def get_language_default(self, language):
        pass
    def get_language(self, extension):
        extension = extension.lower().strip('.')
        for language in self.languages:
            if extension in language.extensions:
                return language
        return None
        
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
        
def create_base_style():
    base_style = Style(None, stc.STC_STYLE_DEFAULT, 'Global Style', None, 
        util.get_font(), 10, False, False, False, 
        (0,0,0), (255,255,255))
    return base_style
        
def create_app_styles(parent):
    app_styles = [
        Style(parent, stc.STC_STYLE_BRACELIGHT, 'Brace (Matched)'),
        Style(parent, stc.STC_STYLE_BRACEBAD, 'Brace (Unmatched)'),
        Style(parent, stc.STC_STYLE_CONTROLCHAR, 'Control Character'),
        Style(parent, stc.STC_STYLE_INDENTGUIDE, 'Indentation Guides'),
        Style(parent, stc.STC_STYLE_LINENUMBER, 'Line Number Margin'),
    ]
    return app_styles
    
def create_languages(base_style):
    result = []
    
    # Python
    style = Style(base_style, name='Python Base Style')
    python = Language(
        name='Python',
        extensions=['py', 'pyw'],
        lexer=stc.STC_LEX_PYTHON,
        base_style=style,
        styles=[
            Style(style, stc.STC_P_CHARACTER, 'Character'),
            Style(style, stc.STC_P_CLASSNAME, 'Classname'),
            Style(style, stc.STC_P_COMMENTBLOCK, 'Comment Block'),
            Style(style, stc.STC_P_COMMENTLINE, 'Comment Line'),
            Style(style, stc.STC_P_DECORATOR, 'Decorator'),
            Style(style, stc.STC_P_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_P_DEFNAME, 'Defname'),
            Style(style, stc.STC_P_IDENTIFIER, 'Identifier'),
            Style(style, stc.STC_P_NUMBER, 'Number'),
            Style(style, stc.STC_P_OPERATOR, 'Operator'),
            Style(style, stc.STC_P_STRING, 'String'),
            Style(style, stc.STC_P_STRINGEOL, 'String EOL'),
            Style(style, stc.STC_P_TRIPLE, 'Triple'),
            Style(style, stc.STC_P_TRIPLEDOUBLE, 'Tripledouble'),
            Style(style, stc.STC_P_WORD, 'Keyword'),
            Style(style, stc.STC_P_WORD2, 'Keyword 2'),
        ],
        keywords='''
            and del from not while as elif global or with assert else if pass yield
            break except import print class exec in raise continue finally is return
            def for lambda try
        ''',
        keywords2='''
            ArithmeticError AssertionError AttributeError
            BaseException DeprecationWarning EOFError Ellipsis EnvironmentError
            Exception False FloatingPointError FutureWarning GeneratorExit IOError
            ImportError ImportWarning IndentationError IndexError KeyError
            KeyboardInterrupt LookupError MemoryError NameError None NotImplemented
            NotImplementedError OSError OverflowError PendingDeprecationWarning
            ReferenceError RuntimeError RuntimeWarning StandardError
            StopIteration SyntaxError SyntaxWarning SystemError SystemExit
            TabError True TypeError UnboundLocalError UnicodeDecodeError
            UnicodeEncodeError UnicodeError UnicodeTranslateError UnicodeWarning
            UserWarning ValueError Warning WindowsError ZeroDivisionError
            __debug__ __doc__ __import__ __name__ abs all any apply basestring
            bool buffer callable chr classmethod cmp coerce compile complex
            copyright credits delattr dict dir divmod enumerate eval execfile
            exit file filter float frozenset getattr globals hasattr hash help
            hex id input int intern isinstance issubclass iter len license list
            locals long map max min object oct open ord pow property quit range
            raw_input reduce reload repr reversed round set setattr slice sorted
            staticmethod str sum super tuple type unichr unicode vars xrange zip
        ''',
        line_comment='#',
        block_comment=("'''", "'''"),
    )
    result.append(python)
    
    # Java
    style = Style(base_style, name='Java Base Style')
    java = Language(
        name='Java',
        extensions=['java'],
        lexer=stc.STC_LEX_CPP,
        base_style=style,
        styles=[
            Style(style, stc.STC_C_CHARACTER, 'Character'),
            Style(style, stc.STC_C_COMMENT, 'Comment'),
            Style(style, stc.STC_C_COMMENTDOC, 'Commentdoc'),
            Style(style, stc.STC_C_COMMENTDOCKEYWORD, 'Commentdockeyword'),
            Style(style, stc.STC_C_COMMENTDOCKEYWORDERROR, 'Commentdockeyworderror'),
            Style(style, stc.STC_C_COMMENTLINE, 'Comment Line'),
            Style(style, stc.STC_C_COMMENTLINEDOC, 'Commentlinedoc'),
            Style(style, stc.STC_C_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_C_IDENTIFIER, 'Identifier'),
            Style(style, stc.STC_C_NUMBER, 'Number'),
            Style(style, stc.STC_C_OPERATOR, 'Operator'),
            Style(style, stc.STC_C_STRING, 'String'),
            Style(style, stc.STC_C_STRINGEOL, 'String EOL'),
            Style(style, stc.STC_C_WORD, 'Keyword'),
            Style(style, stc.STC_C_WORD2, 'Keyword 2'),
        ],
        keywords='''
            abstract continue for new switch assert default goto package synchronized
            boolean do if private this break double implements protected throw
            byte else import public throws case enum instanceof return transient
            catch extends int short try char final interface static void
            class finally long strictfp volatile const float native super while
        ''',
        keywords2='''
        ''',
        keywords3='''
            author code docRoot deprecated exception
            inheritDoc link linkplain literal param return see
            serial serialData serialField since throws value version
        ''',
        line_comment='//',
        block_comment=('/*', '*/'),
    )
    result.append(java)
    
    # C
    style = Style(base_style, name='C Base Style')
    c = Language(
        name='C',
        extensions=['c'],
        lexer=stc.STC_LEX_CPP,
        base_style=style,
        styles=[
            Style(style, stc.STC_C_CHARACTER, 'Character'),
            Style(style, stc.STC_C_COMMENT, 'Comment'),
            Style(style, stc.STC_C_COMMENTLINE, 'Comment Line'),
            Style(style, stc.STC_C_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_C_IDENTIFIER, 'Identifier'),
            Style(style, stc.STC_C_NUMBER, 'Number'),
            Style(style, stc.STC_C_OPERATOR, 'Operator'),
            Style(style, stc.STC_C_PREPROCESSOR, 'Preprocessor'),
            Style(style, stc.STC_C_REGEX, 'Regex'),
            Style(style, stc.STC_C_STRING, 'String'),
            Style(style, stc.STC_C_STRINGEOL, 'String EOL'),
            Style(style, stc.STC_C_UUID, 'Uuid'),
            Style(style, stc.STC_C_VERBATIM, 'Verbatim'),
            Style(style, stc.STC_C_WORD, 'Keyword'),
            Style(style, stc.STC_C_WORD2, 'Keyword 2'),
        ],
        keywords='''
            auto break case char const continue default do double else enum 
            extern float for goto if int long register return short signed sizeof
            static struct switch typedef union unsigned void volatile while
        ''',
        keywords2='''
        ''',
        keywords3='''
        ''',
        line_comment='//',
        block_comment=('/*', '*/'),
    )
    result.append(c)
    
    # C++
    style = Style(base_style, name='C++ Base Style')
    cpp = Language(
        name='C++',
        extensions=['h', 'hpp', 'hxx', 'cpp', 'cxx', 'cc'],
        lexer=stc.STC_LEX_CPP,
        base_style=style,
        styles=[
            Style(style, stc.STC_C_CHARACTER, 'Character'),
            Style(style, stc.STC_C_COMMENT, 'Comment'),
            Style(style, stc.STC_C_COMMENTDOC, 'Commentdoc'),
            Style(style, stc.STC_C_COMMENTDOCKEYWORD, 'Commentdockeyword'),
            Style(style, stc.STC_C_COMMENTDOCKEYWORDERROR, 'Commentdockeyworderror'),
            Style(style, stc.STC_C_COMMENTLINE, 'Comment Line'),
            Style(style, stc.STC_C_COMMENTLINEDOC, 'Commentlinedoc'),
            Style(style, stc.STC_C_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_C_GLOBALCLASS, 'Globalclass'),
            Style(style, stc.STC_C_IDENTIFIER, 'Identifier'),
            Style(style, stc.STC_C_NUMBER, 'Number'),
            Style(style, stc.STC_C_OPERATOR, 'Operator'),
            Style(style, stc.STC_C_PREPROCESSOR, 'Preprocessor'),
            Style(style, stc.STC_C_REGEX, 'Regex'),
            Style(style, stc.STC_C_STRING, 'String'),
            Style(style, stc.STC_C_STRINGEOL, 'String EOL'),
            Style(style, stc.STC_C_UUID, 'Uuid'),
            Style(style, stc.STC_C_VERBATIM, 'Verbatim'),
            Style(style, stc.STC_C_WORD, 'Keyword'),
            Style(style, stc.STC_C_WORD2, 'Keyword 2'),
        ],
        keywords='''
            asm auto bool
            break case catch char class const const_cast continue default delete
            do double dynamic_cast else enum explicit export extern false float for
            friend goto if inline int long mutable namespace new operator private
            protected public register reinterpret_cast return short signed sizeof
            static static_cast struct switch template this throw true try typedef
            typeid typename union unsigned using virtual void volatile wchar_t while
        ''',
        keywords2='''
        ''',
        keywords3='''
        ''',
        line_comment='//',
        block_comment=('/*', '*/'),
    )
    result.append(cpp)
    
    # CSS
    style = Style(base_style, name='CSS Base Style')
    css = Language(
        name='CSS',
        extensions=['css'],
        lexer=stc.STC_LEX_CSS,
        base_style=style,
        styles=[
            Style(style, stc.STC_CSS_ATTRIBUTE, 'Attribute'),
            Style(style, stc.STC_CSS_CLASS, 'Class'),
            Style(style, stc.STC_CSS_COMMENT, 'Comment'),
            Style(style, stc.STC_CSS_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_CSS_DIRECTIVE, 'Directive'),
            Style(style, stc.STC_CSS_DOUBLESTRING, 'Doublestring'),
            Style(style, stc.STC_CSS_ID, 'ID'),
            Style(style, stc.STC_CSS_IDENTIFIER, 'Identifier'),
            Style(style, stc.STC_CSS_IDENTIFIER2, 'Identifier2'),
            Style(style, stc.STC_CSS_IMPORTANT, 'Important'),
            Style(style, stc.STC_CSS_OPERATOR, 'Operator'),
            Style(style, stc.STC_CSS_PSEUDOCLASS, 'Pseudoclass'),
            Style(style, stc.STC_CSS_SINGLESTRING, 'Singlestring'),
            Style(style, stc.STC_CSS_TAG, 'Tag'),
            Style(style, stc.STC_CSS_VALUE, 'Value'),
        ],
        keywords='''
            azimuth background
            background-attachment background-color background-image background-position
            background-repeat border border-bottom border-bottom-color border-bottom-style
            border-bottom-width border-collapse border-color border-left border-left-color
            border-left-style border-left-width border-right border-right-color
            border-right-style border-right-width border-spacing border-style border-top
            border-top-color border-top-style border-top-width border-width bottom
            caption-side clear clip color content counter-increment counter-reset cue
            cue-after cue-before cursor direction display elevation empty-cells float
            font font-family font-size font-size-adjust font-stretch font-style
            font-variant font-weight height left letter-spacing line-height list-style
            list-style-image list-style-position list-style-type margin margin-bottom
            margin-left margin-right margin-top marker-offset marks max-height
            max-width min-height min-width orphans outline outline-color outline-style
            outline-width overflow padding padding-bottom padding-left padding-right
            padding-top page page-break-after page-break-before page-break-inside
            pause pause-after pause-before pitch pitch-range play-during
            position quotes richness right size speak speak-header speak-numeral
            speak-ponctuation speech-rate stress table-layout text-align text-decoration
            text-indent text-shadow text-transform top unicode-bidi vertical-align
            visibility voice-family volume white-space widows width word-spacing z-index
        ''',
        line_comment='',
        block_comment=('/*', '*/'),
    )
    result.append(css)
    
    # HTML
    style = Style(base_style, name='HTML Base Style')
    html = Language(
        name='HTML',
        extensions=['html', 'htm', 'shtml', 'shtm', 'xhtml'],
        lexer=stc.STC_LEX_HTML,
        base_style=style,
        styles=[
            Style(style, stc.STC_H_ATTRIBUTE, 'Attribute'),
            Style(style, stc.STC_H_ATTRIBUTEUNKNOWN, 'Attribute Unknown'),
            Style(style, stc.STC_H_CDATA, 'CDATA'),
            Style(style, stc.STC_H_COMMENT, 'Comment'),
            Style(style, stc.STC_H_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_H_DOUBLESTRING, 'Double String'),
            Style(style, stc.STC_H_ENTITY, 'Entity'),
            Style(style, stc.STC_H_NUMBER, 'Number'),
            Style(style, stc.STC_H_OTHER, 'Other'),
            Style(style, stc.STC_H_QUESTION, 'Question'),
            Style(style, stc.STC_H_SCRIPT, 'Script'),
            Style(style, stc.STC_H_SINGLESTRING, 'Single String'),
            Style(style, stc.STC_H_TAG, 'Tag'),
            Style(style, stc.STC_H_TAGEND, 'Tag End'),
            Style(style, stc.STC_H_TAGUNKNOWN, 'Tag Unknown'),
            Style(style, stc.STC_H_VALUE, 'Value'),
            Style(style, stc.STC_H_XCCOMMENT, 'Xccomment'),
            Style(style, stc.STC_H_XMLEND, 'XML End'),
            Style(style, stc.STC_H_XMLSTART, 'XML Start'),
        ],
        keywords='''
        ''',
        line_comment='',
        block_comment=('<!--', '-->'),
    )
    result.append(html)

    # XML
    style = Style(base_style, name='XML Base Style')
    xml = Language(
        name='XML',
        extensions=['xml', 'spp'],
        lexer=stc.STC_LEX_XML,
        base_style=style,
        styles=[
            Style(style, stc.STC_H_ATTRIBUTE, 'Attribute'),
            Style(style, stc.STC_H_ATTRIBUTEUNKNOWN, 'Attribute Unknown'),
            Style(style, stc.STC_H_CDATA, 'CDATA'),
            Style(style, stc.STC_H_COMMENT, 'Comment'),
            Style(style, stc.STC_H_DEFAULT, 'Whitespace'),
            Style(style, stc.STC_H_DOUBLESTRING, 'Double String'),
            Style(style, stc.STC_H_ENTITY, 'Entity'),
            Style(style, stc.STC_H_NUMBER, 'Number'),
            Style(style, stc.STC_H_OTHER, 'Other'),
            Style(style, stc.STC_H_QUESTION, 'Question'),
            Style(style, stc.STC_H_SCRIPT, 'Script'),
            Style(style, stc.STC_H_SINGLESTRING, 'Single String'),
            Style(style, stc.STC_H_TAG, 'Tag'),
            Style(style, stc.STC_H_TAGEND, 'Tag End'),
            Style(style, stc.STC_H_TAGUNKNOWN, 'Tag Unknown'),
            Style(style, stc.STC_H_VALUE, 'Value'),
            Style(style, stc.STC_H_XCCOMMENT, 'Xccomment'),
            Style(style, stc.STC_H_XMLEND, 'XML End'),
            Style(style, stc.STC_H_XMLSTART, 'XML Start'),
        ],
        keywords='''
        ''',
        line_comment='',
        block_comment=('<!--', '-->'),
    )
    result.append(xml)
    
    # Makefile
    style = Style(base_style, name='Makefile Base Style')
    makefile = Language(
        name='Makefile',
        extensions=['makefile'],
        lexer=stc.STC_LEX_MAKEFILE,
        base_style=style,
        styles=[
            Style(style, stc.STC_MAKE_COMMENT, 'Comment'),
            Style(style, stc.STC_MAKE_DEFAULT, 'Default'),
            Style(style, stc.STC_MAKE_IDENTIFIER, 'Identifer'),
            Style(style, stc.STC_MAKE_IDEOL, 'Ideol'),
            Style(style, stc.STC_MAKE_OPERATOR, 'Operator'),
            Style(style, stc.STC_MAKE_PREPROCESSOR, 'Preprocessor'),
            Style(style, stc.STC_MAKE_TARGET, 'Make target'),
        ],
        keywords='''
        ''',
        line_comment='#',
        block_comment=('', ''),
    )
    result.append(makefile)

    return result
    
