# Power BI Report Page Configuration

Your ESP32 display is 480x480 pixels with 16-bit color. Here's how to design a report page that looks good on it.

---

## Step 1: Set Custom Page Size (480x480)

1. Open your report in **Power BI Desktop**
2. Click on the canvas background (not on any visual)
3. In the **Visualizations** pane, click the **Format** tab (paint roller icon)
4. Expand **Canvas settings** (or **Page size** in older versions)
5. Set **Type** to **Custom**
6. Set:
   - **Width**: 480 px
   - **Height**: 480 px
7. The canvas will resize to a square

---

## Step 2: Design Tips for a 480px Display

The display is small (4 inches, 480px), so every pixel counts.

### Text & Fonts
- **Minimum font size**: 10pt for body text, 14pt+ for headers
- Use **bold** for important numbers — thin fonts get lost at this resolution
- Stick to **sans-serif fonts** (Segoe UI, Arial, DIN) — serifs blur at low res
- Avoid long text; use abbreviations where possible

### Colors & Contrast
- Use **high contrast** — the display renders 16-bit color (RGB565 = 65K colors), not millions
- **Dark backgrounds** with light text work well on this display
- Avoid subtle gradients — they can show banding in 16-bit color
- Solid, flat colors render cleanest

### Layout
- **Keep it simple** — 3 to 5 visuals maximum
- Cards with big numbers work great (KPIs, totals)
- Simple bar/column charts are readable; avoid scatter plots or dense line charts
- Leave padding between visuals (8-10px minimum)
- Consider the status bar overlay at the bottom (~20px) — keep important content above it

### What Works Well
- KPI cards with large numbers
- Simple bar/column charts (5-8 bars max)
- Gauges and donuts
- Tables with 3-5 rows, 2-3 columns, large font
- Solid color backgrounds

### What to Avoid
- Maps (too much detail for 480px)
- Dense tables (10+ rows)
- Small legends
- Thin line charts with many series
- Waterfall charts
- Anything requiring hover/tooltips (this is a static image)

---

## Step 3: Publish

1. In Power BI Desktop, click **Publish**
2. Select your Premium workspace
3. Open the report in **app.powerbi.com** to verify it looks correct
4. Note the URL to get your Group ID and Report ID (see Azure setup guide)

---

## Step 4: Get Your Page Name (if multi-page)

If your report has multiple pages and you want to export a specific one:

1. Open the report in app.powerbi.com
2. Click the page tab you want
3. Look at the URL — find `pageName=` parameter
4. Or use the Power BI REST API:
   ```
   GET https://api.powerbi.com/v1.0/myorg/groups/{groupId}/reports/{reportId}/pages
   ```
   This returns a list of pages with their `name` values

For a single-page report, you can leave the page name blank — the API will use the default page.

---

## Example Layout (480x480)

```
┌──────────────────────────────────┐
│  DASHBOARD TITLE           12:30 │  ← 40px header
├────────────────┬─────────────────┤
│                │                 │
│   KPI Card 1   │   KPI Card 2   │  ← ~100px
│   Revenue      │   Units Sold   │
│   £1.2M        │   4,523        │
│                │                 │
├────────────────┴─────────────────┤
│                                  │
│     Bar Chart (5 categories)     │  ← ~200px
│     ████████████                 │
│     ██████████                   │
│     ████████                     │
│     ██████                       │
│     ████                         │
│                                  │
├──────────────────────────────────┤
│                                  │
│  Small table or additional KPIs  │  ← ~120px
│                                  │
└──────────────────────────────────┘
                                      ← 20px reserved for status overlay
```
