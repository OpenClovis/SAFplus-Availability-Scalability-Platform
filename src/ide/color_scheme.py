import wx
import wx.richtext as rt
import xml.etree.ElementTree as ET
import os
from copy import deepcopy
from util import get_ide_dir, rbg_to_tuple

def get_schemes():
    scheme_file = get_ide_dir() + os.sep + "schemes.xml"
    schemes_doc = ET.parse(scheme_file)
    schemeElements = schemes_doc.findall("./scheme")
    return [Scheme(element.attrib["name"], schemes_doc) for element in schemeElements]

class SchemeDemo(wx.Panel):
    # Demos will be shown in "Color Scheme" tab
    selected_scheme = None

    def __init__(self, style_dialog, scheme_page, scheme_obj):
        super().__init__(parent = scheme_page, size = (300, 90))

        self.style_dialog = style_dialog
        self.scheme_obj = scheme_obj
        self.v_box = wx.BoxSizer(wx.VERTICAL)
        self.rtc = rt.RichTextCtrl(self, style = wx.TE_READONLY)

        demo_tokens = [
            ("/* {0} */\n".format(self.scheme_obj.name), "Comment"),
            ("#include <safplus.hxx>\n", "Preprocessor"),
            ("int ", "Keyword 2"),
            ("running = ", "Identifier"),
            ("false", "Keyword"),
            (";", "Operator")
        ]

        bg_color = wx.Colour(rbg_to_tuple(self.scheme_obj.background_color))
        self.rtc.SetBackgroundColour(bg_color)

        for (token, category) in demo_tokens:
            style = self.scheme_obj.lang_styles["C++"][category]
            fg_color = wx.Colour(rbg_to_tuple(style.get("foreground")))
            bold = style.get("bold", False)
            italic = style.get("italic", False)

            if bold: self.rtc.BeginBold()
            if italic: self.rtc.BeginItalic()

            self.rtc.BeginTextColour(fg_color)
            self.rtc.WriteText(token)
            self.rtc.EndTextColour()

            if italic: self.rtc.EndItalic()
            if bold: self.rtc.EndBold()

        self.v_box.Add(self.rtc, 1, wx.EXPAND | wx.ALL, 5)
        self.SetSizer(self.v_box)
        self.Fit()
        self.Bind(wx.EVT_LEFT_DOWN, self.on_clicked)
        self.rtc.Bind(wx.EVT_LEFT_DOWN, self.on_clicked)
        self.Bind(wx.EVT_MOUSEWHEEL, self.on_mouse_wheel)
        self.rtc.Bind(wx.EVT_MOUSEWHEEL, self.on_mouse_wheel)

    def focus(self):
        # Indicating the scheme has been selected
        self.SetBackgroundColour(wx.Colour(255, 127, 0))

    def unfocus(self):
        self.SetBackgroundColour(wx.Colour(234, 234, 234))

    def on_clicked(self, evt):
        self.scheme_obj.apply(self.style_dialog)
        self.style_dialog.apply.Enable() # enable the apply button
        self.style_dialog.update_all_controls()

        if SchemeDemo.selected_scheme:
            SchemeDemo.selected_scheme.unfocus()
        self.focus()
        SchemeDemo.selected_scheme = self

    def on_mouse_wheel(self, evt):
        # Scroll the parent instead
        rotation = evt.GetWheelRotation()
        direction = - int(rotation / abs(rotation))
        self.GetParent().ScrollLines(direction)

class Scheme():
    def __init__(self, scheme_name, schemes_doc):
        self.name = scheme_name
        self.font = "Monospace"
        self.size = 11
        self.background_color = None
        self.selection_bg_color = None
        self.selection_fg_color = None
        self.caret_line_bg = None
        self.caret_fg = None
        self.brace_bg_color = None
        self.brace_fg_color = None
        self.lang_styles = {
            "C": None,
            "C++": None,
            "XML": None,
            "Makefile": None,
            "Yang": None
        }
        self.parse(schemes_doc)

    def parse(self, schemes_doc):
        scheme_tag = schemes_doc.find("./scheme[@name='{0}']".format(self.name))
        language_tags = scheme_tag.findall("./language")
        editor_tag = scheme_tag.find("./editor")
        selection_tag = scheme_tag.find("./selection")
        caret_tag = scheme_tag.find("./caret")
        brace_tag = scheme_tag.find("./brace")

        self.background_color = editor_tag.attrib["background"]
        self.selection_bg_color = selection_tag.attrib["background"]
        self.selection_fg_color = selection_tag.attrib["foreground"]
        self.caret_line_bg = caret_tag.attrib["line"]
        self.caret_fg = caret_tag.attrib["foreground"]
        self.brace_bg_color = brace_tag.attrib["background"]
        self.brace_fg_color = brace_tag.attrib["foreground"]

        for language_tag in language_tags:
            lang_name = language_tag.attrib["name"]
            style_tags = language_tag.findall("./style")
            language_styles = {}
            for style_tag in style_tags:
                style_name = style_tag.attrib["name"]
                styles = deepcopy(style_tag.attrib)
                for k,v in styles.items():
                    if v.lower() == "true": v = True
                    elif v.lower() == "false": v = False
                    styles[k] = v
                del styles["name"]
                language_styles[style_name] = styles
            self.lang_styles[lang_name] = language_styles

    def create_demo(self, style_dialog, page):
        demo = SchemeDemo(style_dialog, page, self)
        return demo

    def apply(self, style_dialog):
        for (lang_name, scheme_styles) in self.lang_styles.items():
            if scheme_styles:
                target_lang = self.get_lang_obj(lang_name, style_dialog)
                self.set_lang_styles(target_lang, scheme_styles)

        style_dialog.editor_background_color = rbg_to_tuple(self.background_color)
        style_dialog.selection_bg_color = rbg_to_tuple(self.selection_bg_color)
        style_dialog.selection_fg_color = rbg_to_tuple(self.selection_fg_color)
        style_dialog.caret_line_bg = rbg_to_tuple(self.caret_line_bg)
        style_dialog.caret_fg = rbg_to_tuple(self.caret_fg)

        base_fg_color = self.lang_styles["C++"]["Identifier"]["foreground"]
        style_dialog.base_style.background = rbg_to_tuple(self.background_color)
        style_dialog.base_style.foreground = rbg_to_tuple(base_fg_color)

        brace_style = self.get_app_brace_style(style_dialog)
        brace_style.background = rbg_to_tuple(self.brace_bg_color)
        brace_style.foreground = rbg_to_tuple(self.brace_fg_color)

    def get_lang_obj(self, lang_name, style_dialog):
        for lang in style_dialog.languages:
            if lang_name == lang.name:
                return lang

    def set_lang_styles(self, target_lang_obj, scheme_styles):
        for target_lang_style in target_lang_obj.styles:
            style_name = target_lang_style.name
            scheme_style = scheme_styles[style_name]
            target_lang_style.font = self.font
            target_lang_style.size = self.size
            target_lang_style.bold = scheme_style.get("bold", None)
            target_lang_style.italic = scheme_style.get("italic", None)
            target_lang_style.underline = scheme_style.get("underline", None)

            fg = scheme_style.get("foreground", "#000000")
            target_lang_style.foreground = rbg_to_tuple(fg)
            target_lang_style.background = rbg_to_tuple(self.background_color)

    def get_app_brace_style(self, style_dialog):
        for style in style_dialog.app_styles:
            if style.name == "Brace (Matched)":
                return style
