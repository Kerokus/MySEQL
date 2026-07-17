using myseq.Properties;
using System;
using System.Drawing;
using System.Windows.Forms;

namespace myseq
{
    // Preset color themes for the client window. The map keeps its own colors (Settings.BackColor etc.);
    // this themes the surrounding chrome (form, menus, toolbar, status bar, dock panels, lists) and pushes
    // the list/map color values into Settings so the existing color-driven code + option pickers stay in sync.
    public class ThemePalette
    {
        public string Name;
        public Color FormBack;     // form / dock background
        public Color PanelBack;    // panels, textboxes, toolbar/menu background
        public Color Text;         // general foreground text
        public Color Accent;       // active tab / selection highlight
        public Color ListBack;     // spawn/timer/ground list background
        public Color MapBack, GridColor, GridLabel, RangeCircle, PCBorder; // map colors (Settings)
    }

    public static class ThemeManager
    {
        public static readonly ThemePalette Light = new ThemePalette
        {
            Name = "Light",
            FormBack = SystemColors.Control,
            PanelBack = SystemColors.ControlLightLight,
            Text = SystemColors.ControlText,
            Accent = SystemColors.Highlight,
            ListBack = Color.White,
            MapBack = Color.Black,
            GridColor = Color.DarkGreen,
            GridLabel = Color.Yellow,
            RangeCircle = Color.DarkGray,
            PCBorder = Color.Fuchsia
        };

        public static readonly ThemePalette Dark = new ThemePalette
        {
            Name = "Dark",
            FormBack = Color.FromArgb(37, 37, 38),
            PanelBack = Color.FromArgb(45, 45, 48),
            Text = Color.FromArgb(220, 220, 220),
            Accent = Color.FromArgb(62, 62, 66),
            ListBack = Color.FromArgb(30, 30, 30),
            MapBack = Color.Black,
            GridColor = Color.DarkGreen,
            GridLabel = Color.Yellow,
            RangeCircle = Color.DarkGray,
            PCBorder = Color.Fuchsia
        };

        public static ThemePalette[] All => new[] { Light, Dark };

        public static ThemePalette ByName(string name)
            => string.Equals(name, "Dark", StringComparison.OrdinalIgnoreCase) ? Dark : Light;

        public static ThemePalette Current => ByName(Settings.Default.ThemeName);

        // Copy a palette's map/list color values into Settings (option pickers + map code read these).
        public static void ApplyToSettings(ThemePalette p)
        {
            Settings.Default.ThemeName = p.Name;
            Settings.Default.ListBackColor = p.ListBack;
            Settings.Default.BackColor = p.MapBack;
            Settings.Default.GridColor = p.GridColor;
            Settings.Default.GridLabelColor = p.GridLabel;
            Settings.Default.RangeCircleColor = p.RangeCircle;
            Settings.Default.PCBorderColor = p.PCBorder;
        }

        // Theme the window chrome. Call on load and after the options dialog closes.
        public static void ApplyChrome(Form form)
        {
            var p = Current;
            form.BackColor = p.FormBack;
            form.ForeColor = p.Text;
            foreach (Control c in form.Controls)
                ThemeControl(c, p);
        }

        private static void ThemeControl(Control c, ThemePalette p)
        {
            // Leave the map surface alone (it draws its own colors from Settings).
            var typeName = c.GetType().Name;
            if (typeName.StartsWith("Map"))
                return;

            switch (c)
            {
                case MenuStrip ms:
                    ms.BackColor = p.PanelBack; ms.ForeColor = p.Text;
                    ms.Renderer = new ThemeRenderer(p);
                    break;
                case StatusStrip ss:
                    ss.BackColor = p.PanelBack; ss.ForeColor = p.Text;
                    ss.Renderer = new ThemeRenderer(p);
                    break;
                case ToolStrip ts:
                    ts.BackColor = p.PanelBack; ts.ForeColor = p.Text;
                    ts.Renderer = new ThemeRenderer(p);
                    break;
                case ListView lv:
                    lv.BackColor = p.ListBack; lv.ForeColor = p.Text;
                    break;
                case TextBox tb:
                    tb.BackColor = p.PanelBack; tb.ForeColor = p.Text;
                    break;
                default:
                    c.BackColor = p.FormBack; c.ForeColor = p.Text;
                    break;
            }

            foreach (Control child in c.Controls)
                ThemeControl(child, p);
        }

        // Dark-aware renderer for menus/toolbar/status bar.
        private class ThemeRenderer : ToolStripProfessionalRenderer
        {
            public ThemeRenderer(ThemePalette p) : base(new ThemeColorTable(p)) { RoundedEdges = false; }
        }

        private class ThemeColorTable : ProfessionalColorTable
        {
            private readonly ThemePalette p;
            public ThemeColorTable(ThemePalette pal) { p = pal; }

            public override Color ToolStripGradientBegin => p.PanelBack;
            public override Color ToolStripGradientMiddle => p.PanelBack;
            public override Color ToolStripGradientEnd => p.PanelBack;
            public override Color MenuStripGradientBegin => p.PanelBack;
            public override Color MenuStripGradientEnd => p.PanelBack;
            public override Color MenuItemSelected => p.Accent;
            public override Color MenuItemSelectedGradientBegin => p.Accent;
            public override Color MenuItemSelectedGradientEnd => p.Accent;
            public override Color MenuItemBorder => p.Accent;
            public override Color MenuBorder => p.Accent;
            public override Color MenuItemPressedGradientBegin => p.PanelBack;
            public override Color MenuItemPressedGradientEnd => p.PanelBack;
            public override Color ImageMarginGradientBegin => p.PanelBack;
            public override Color ImageMarginGradientMiddle => p.PanelBack;
            public override Color ImageMarginGradientEnd => p.PanelBack;
            public override Color ToolStripBorder => p.FormBack;
            public override Color SeparatorDark => p.FormBack;
            public override Color SeparatorLight => p.Accent;
            public override Color ButtonSelectedGradientBegin => p.Accent;
            public override Color ButtonSelectedGradientEnd => p.Accent;
            public override Color ButtonSelectedHighlight => p.Accent;
            public override Color StatusStripGradientBegin => p.PanelBack;
            public override Color StatusStripGradientEnd => p.PanelBack;
        }
    }
}
